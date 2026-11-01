#include "geo_format.h"

#include <cmath>
#include <cstdio>

namespace tracker {

const char* const kNoLatitude = " --  --.--' -";
const char* const kNoLongitude = "---  --.--' -";
const char* const kNoSpeed = "--.-KT";
const char* const kNoCourse = "---";
const char* const kNoDateTime = "--:-- --.--.----";

DegreesMinutes toDegreesMinutes(double decimalDegrees, bool isLatitude) {
    DegreesMinutes result{};
    if (isLatitude) {
        result.hemisphere = decimalDegrees < 0 ? 'S' : 'N';
    } else {
        result.hemisphere = decimalDegrees < 0 ? 'W' : 'E';
    }

    const double absolute = std::fabs(decimalDegrees);
    result.degrees = static_cast<uint16_t>(absolute);
    const double minutes = (absolute - result.degrees) * 60.0;
    result.minutes = static_cast<uint8_t>(minutes);
    long hundredths = static_cast<long>(std::ceil((minutes - result.minutes) * 100.0));
    result.hundredths = static_cast<uint8_t>(hundredths >= 100 ? 99 : hundredths);
    return result;
}

void formatLatitude(char* out, size_t size, double decimalDegrees) {
    const DegreesMinutes dm = toDegreesMinutes(decimalDegrees, true);
    std::snprintf(out, size, " %02u  %02u.%02u' %c", static_cast<unsigned>(dm.degrees),
                  static_cast<unsigned>(dm.minutes), static_cast<unsigned>(dm.hundredths),
                  dm.hemisphere);
}

void formatLongitude(char* out, size_t size, double decimalDegrees) {
    const DegreesMinutes dm = toDegreesMinutes(decimalDegrees, false);
    std::snprintf(out, size, "%03u  %02u.%02u' %c", static_cast<unsigned>(dm.degrees),
                  static_cast<unsigned>(dm.minutes), static_cast<unsigned>(dm.hundredths),
                  dm.hemisphere);
}

void formatSpeed(char* out, size_t size, float knots) {
    long tenths = std::lround(std::fabs(knots) * 10.0f);
    if (tenths > 999) {
        tenths = 999;
    }
    std::snprintf(out, size, "%02ld.%ldKT", tenths / 10, tenths % 10);
}

void formatCourse(char* out, size_t size, float degrees) {
    long whole = std::lround(degrees) % 360;
    if (whole < 0) {
        whole += 360;
    }
    std::snprintf(out, size, "%03ld", whole);
}

}  // namespace tracker
