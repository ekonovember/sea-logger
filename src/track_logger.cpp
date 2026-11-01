#include "track_logger.h"

#include <SD.h>

bool TrackLogger::begin() { return SD.begin(chipSelectPin_); }

bool TrackLogger::add(const tracker::TrackPoint& point) { return buffer_.push(point); }

bool TrackLogger::flush() {
    if (buffer_.empty()) {
        return true;
    }

    // Points within one flush can straddle midnight UTC: each one goes to the
    // file of its own day, opening a new file only when the day changes.
    char fileName[20];
    char openName[20] = "";
    char line[96];
    File file;

    for (size_t i = 0; i < buffer_.size(); ++i) {
        const tracker::TrackPoint& point = buffer_.at(i);
        if (tracker::formatFileName(fileName, sizeof fileName, point.time) == 0) {
            continue;
        }
        if (strcmp(fileName, openName) != 0) {
            if (file) {
                file.close();
            }
            const bool isNew = !SD.exists(fileName);
            file = SD.open(fileName, FILE_WRITE);
            if (!file) {
                return false;  // keep the buffer, try again next time
            }
            if (isNew) {
                file.print(tracker::kCsvHeader);
            }
            strcpy(openName, fileName);
        }
        const size_t length = tracker::formatCsvLine(line, sizeof line, point);
        if (length > 0) {
            file.write(reinterpret_cast<const uint8_t*>(line), length);
        }
    }

    if (file) {
        file.close();
    }
    written_ += buffer_.size();
    buffer_.clear();
    return true;
}
