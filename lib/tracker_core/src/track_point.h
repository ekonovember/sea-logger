#pragma once
// One logged position, its CSV representation and the logging policy.

#include <cstddef>
#include <cstdint>

namespace tracker {

struct UtcDateTime {
    uint16_t year;  // full year, e.g. 2026
    uint8_t month;  // 1..12
    uint8_t day;    // 1..31
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct TrackPoint {
    double latitude;   // decimal degrees, + north
    double longitude;  // decimal degrees, + east
    float sogKnots;    // speed over ground
    float cogDegrees;  // course over ground, true
    UtcDateTime time;
};

// What the receiver currently knows — the inputs of the logging decision.
struct FixQuality {
    bool hasLocation;
    bool hasDateTime;
    bool hasErrorEstimate;  // needs the GST sentence enabled on the receiver
    float latitudeErrorM;
    float longitudeErrorM;
};

struct LoggingPolicy {
    float maxErrorM;          // mean of lat/lon 1-sigma error must not exceed this
    bool requireMovement;     // log only while under way (see MotionDetector)
};

// Mean horizontal error in metres, or a negative value when unknown.
float meanErrorM(const FixQuality& fix);

// A fix is logged only when it has a position, a timestamp and a known error
// within the policy's limit, and — if the policy requires it — while moving.
bool shouldLog(const FixQuality& fix, bool moving, const LoggingPolicy& policy);

// Header line written once at the top of every log file.
extern const char* const kCsvHeader;

// "2026-11-01T14:03:27Z,50.123456,19.123456,5.3,123.0" (+ newline).
// Returns the number of characters written (excluding the terminator), or 0
// if the buffer is too small.
size_t formatCsvLine(char* out, size_t size, const TrackPoint& point);

// One file per UTC day: "/2026-11-01.csv". Returns 0 if the buffer is too small.
size_t formatFileName(char* out, size_t size, const UtcDateTime& time);

// "14:03 01.11.2026" for the screen.
void formatScreenDateTime(char* out, size_t size, const UtcDateTime& time);

}  // namespace tracker
