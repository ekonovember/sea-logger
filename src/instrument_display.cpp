#include "instrument_display.h"

#include <stdio.h>

#include "fonts/monospaced_plain_10.h"
#include "geo_format.h"
#include "gps_receiver.h"

namespace {
constexpr int kWidth = 128;
constexpr int kAccuracyX = 119;  // centre of the accuracy / under-way indicator
constexpr int kAccuracyY = 51;
}  // namespace

InstrumentDisplay::InstrumentDisplay(uint8_t i2cAddress, uint8_t sdaPin, uint8_t sclPin)
    : oled_(i2cAddress, sdaPin, sclPin) {}

bool InstrumentDisplay::begin() {
    if (!oled_.init()) {
        return false;
    }
    oled_.flipScreenVertically();
    oled_.setFont(Monospaced_plain_10);
    oled_.setTextAlignment(TEXT_ALIGN_LEFT);
    oled_.clear();
    oled_.display();
    return true;
}

void InstrumentDisplay::showStatus(const char* line1, const char* line2) {
    oled_.clear();
    oled_.drawString(0, 18, line1);
    oled_.drawString(0, 34, line2);
    oled_.display();
}

void InstrumentDisplay::showFix(const GpsReading& r, bool underWay) {
    using namespace tracker;

    char dateTime[24];
    char course[8];
    char speed[12];
    char latitude[20];
    char longitude[20];

    if (r.quality.hasDateTime) {
        formatScreenDateTime(dateTime, sizeof dateTime, r.point.time);
    } else {
        snprintf(dateTime, sizeof dateTime, "%s", kNoDateTime);
    }
    if (r.hasCourse) {
        formatCourse(course, sizeof course, r.point.cogDegrees);
    } else {
        snprintf(course, sizeof course, "%s", kNoCourse);
    }
    if (r.hasSpeed) {
        formatSpeed(speed, sizeof speed, r.point.sogKnots);
    } else {
        snprintf(speed, sizeof speed, "%s", kNoSpeed);
    }
    if (r.quality.hasLocation) {
        formatLatitude(latitude, sizeof latitude, r.point.latitude);
        formatLongitude(longitude, sizeof longitude, r.point.longitude);
    } else {
        snprintf(latitude, sizeof latitude, "%s", kNoLatitude);
        snprintf(longitude, sizeof longitude, "%s", kNoLongitude);
    }

    oled_.clear();

    // Time
    oled_.drawString(0, 0, String("UTC: ") + dateTime);

    // Course and speed
    oled_.drawHorizontalLine(0, 15, kWidth);
    oled_.drawString(0, 20, String("COG: ") + course);
    oled_.drawCircle(52, 25, 2);  // degree sign
    oled_.drawVerticalLine(57, 15, 20);
    oled_.drawString(60, 20, String("SOG: ") + speed);

    // Position
    oled_.drawHorizontalLine(0, 35, kWidth);
    oled_.drawString(0, 40, String("lat: ") + latitude);
    oled_.drawCircle(52, 45, 2);
    oled_.drawString(0, 50, String("lng: ") + longitude);
    oled_.drawCircle(52, 55, 2);

    // Logging indicator: concentric rings while recording under way,
    // otherwise the mean position error in metres inside a circle.
    if (underWay) {
        oled_.drawCircle(kAccuracyX, kAccuracyY, 1);
        oled_.drawCircle(kAccuracyX, kAccuracyY, 3);
        oled_.drawCircle(kAccuracyX, kAccuracyY, 5);
    } else {
        const float error = meanErrorM(r.quality);
        char accuracy[4] = "--";
        if (error >= 0.0f && error < 99.5f) {
            snprintf(accuracy, sizeof accuracy, "%02d", static_cast<int>(error + 0.5f));
        }
        oled_.drawString(kAccuracyX - 6, kAccuracyY - 6, accuracy);
        oled_.drawCircle(kAccuracyX, kAccuracyY, 8);
    }

    oled_.display();
}

void InstrumentDisplay::toggle() {
    on_ = !on_;
    if (on_) {
        oled_.displayOn();
    } else {
        oled_.displayOff();
    }
}
