#include "track_point.h"

#include <cstdio>

namespace tracker {

const char* const kCsvHeader = "time_utc,latitude,longitude,sog_kn,cog_deg\n";

float meanErrorM(const FixQuality& fix) {
    if (!fix.hasErrorEstimate) {
        return -1.0f;
    }
    return (fix.latitudeErrorM + fix.longitudeErrorM) / 2.0f;
}

bool shouldLog(const FixQuality& fix, bool moving, const LoggingPolicy& policy) {
    if (!fix.hasLocation || !fix.hasDateTime) {
        return false;
    }
    const float error = meanErrorM(fix);
    if (error < 0.0f || error > policy.maxErrorM) {
        return false;
    }
    return moving || !policy.requireMovement;
}

static size_t checked(int written, size_t size) {
    if (written < 0 || static_cast<size_t>(written) >= size) {
        return 0;
    }
    return static_cast<size_t>(written);
}

size_t formatCsvLine(char* out, size_t size, const TrackPoint& p) {
    const int written = std::snprintf(
        out, size, "%04u-%02u-%02uT%02u:%02u:%02uZ,%.6f,%.6f,%.1f,%.1f\n",
        static_cast<unsigned>(p.time.year), static_cast<unsigned>(p.time.month),
        static_cast<unsigned>(p.time.day), static_cast<unsigned>(p.time.hour),
        static_cast<unsigned>(p.time.minute), static_cast<unsigned>(p.time.second), p.latitude,
        p.longitude, static_cast<double>(p.sogKnots), static_cast<double>(p.cogDegrees));
    return checked(written, size);
}

size_t formatFileName(char* out, size_t size, const UtcDateTime& t) {
    const int written =
        std::snprintf(out, size, "/%04u-%02u-%02u.csv", static_cast<unsigned>(t.year),
                      static_cast<unsigned>(t.month), static_cast<unsigned>(t.day));
    return checked(written, size);
}

void formatScreenDateTime(char* out, size_t size, const UtcDateTime& t) {
    std::snprintf(out, size, "%02u:%02u %02u.%02u.%04u", static_cast<unsigned>(t.hour),
                  static_cast<unsigned>(t.minute), static_cast<unsigned>(t.day),
                  static_cast<unsigned>(t.month), static_cast<unsigned>(t.year));
}

}  // namespace tracker
