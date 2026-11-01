#pragma once
// Text formatting for the instrument screen and the log files.
// Pure C++, no Arduino headers: compiled for the board and for the native tests.

#include <cstddef>
#include <cstdint>

namespace tracker {

// A coordinate in degrees + decimal minutes, the notation used on nautical charts.
struct DegreesMinutes {
    uint16_t degrees;     // 0..180
    uint8_t minutes;      // 0..59
    uint8_t hundredths;   // hundredths of a minute, 0..99
    char hemisphere;      // 'N' / 'S' for latitude, 'E' / 'W' for longitude
};

// Converts signed decimal degrees to degrees and hundredths of a minute.
DegreesMinutes toDegreesMinutes(double decimalDegrees, bool isLatitude);

// " 50  15.23' N" — 13 characters, fixed width so the screen layout never shifts.
void formatLatitude(char* out, size_t size, double decimalDegrees);
// "019  02.07' E"
void formatLongitude(char* out, size_t size, double decimalDegrees);

// Speed over ground: "05.3KT"; values above 99.9 kn are clamped.
void formatSpeed(char* out, size_t size, float knots);
// Course over ground in whole degrees: "007", "359"; 360 wraps to "000".
void formatCourse(char* out, size_t size, float degrees);

// Placeholders shown until the receiver delivers the value.
extern const char* const kNoLatitude;
extern const char* const kNoLongitude;
extern const char* const kNoSpeed;
extern const char* const kNoCourse;
extern const char* const kNoDateTime;

}  // namespace tracker
