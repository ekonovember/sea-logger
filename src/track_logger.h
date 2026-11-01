#pragma once
// Buffers track points in RAM and appends them to a per-day CSV file on the SD
// card in batches — one file open per minute instead of one per fix, which
// saves power and card wear.

#include "config.h"
#include "ring_buffer.h"
#include "track_point.h"

class TrackLogger {
public:
    explicit TrackLogger(uint8_t chipSelectPin) : chipSelectPin_(chipSelectPin) {}

    bool begin();

    // Returns false if the buffer was full and the oldest point was dropped.
    bool add(const tracker::TrackPoint& point);

    // Appends everything buffered to the day's file and empties the buffer.
    bool flush();

    size_t pending() const { return buffer_.size(); }
    uint32_t written() const { return written_; }

private:
    uint8_t chipSelectPin_;
    tracker::RingBuffer<tracker::TrackPoint, config::kTrackBufferCapacity> buffer_;
    uint32_t written_ = 0;
};
