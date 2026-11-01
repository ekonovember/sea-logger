// GPS track logger for sailors.
//
// Wiring and behaviour are described in README.md; every tunable lives in
// include/config.h. This file only wires the modules together:
//
//   GpsReceiver ──> MotionDetector ──> shouldLog() ──> TrackLogger ──> SD card
//        └────────────────────────────> InstrumentDisplay
//
// The loop never blocks: the GPS delivers one fix per second and every other
// job runs on its own IntervalTimer.

#include <Arduino.h>

#include "config.h"
#include "gps_receiver.h"
#include "instrument_display.h"
#include "motion_detector.h"
#include "timing.h"
#include "track_logger.h"

namespace {

GpsReceiver gps(config::kGpsRxPin, config::kGpsTxPin);
InstrumentDisplay display(config::kDisplayI2cAddress, SDA, SCL);
TrackLogger logger(config::kSdChipSelectPin);

tracker::MotionDetector motion(config::kMotionStartMs, config::kMotionStopMs);
tracker::ButtonDebouncer button(config::kButtonSettleMs);
tracker::IntervalTimer displayTimer(config::kDisplayRefreshMs);
tracker::IntervalTimer logTimer(config::kLogIntervalMs);
tracker::IntervalTimer flushTimer(config::kSdFlushIntervalMs);

const tracker::LoggingPolicy kPolicy{config::kMaxPositionErrorM, config::kRequireMovement};

GpsReading reading;

void debugPrint(const GpsReading& r) {
    if (!config::kSerialDebug) {
        return;
    }
    Serial.printf("fix loc=%d time=%d err=%.1fm sog=%.1fkn cog=%.0f moving=%d buffered=%u\n",
                  r.quality.hasLocation, r.quality.hasDateTime,
                  static_cast<double>(tracker::meanErrorM(r.quality)),
                  static_cast<double>(r.point.sogKnots), static_cast<double>(r.point.cogDegrees),
                  motion.isMoving(), static_cast<unsigned>(logger.pending()));
}

// Retry a peripheral until it answers, telling the user which one is missing.
template <typename Init>
void startOrWait(const char* name, Init init) {
    while (!init()) {
        display.showStatus(name, "not responding...");
        if (config::kSerialDebug) {
            Serial.printf("%s not responding, retrying\n", name);
        }
        delay(2000);
    }
}

}  // namespace

void setup() {
    if (config::kSerialDebug) {
        Serial.begin(config::kSerialBaud);
    }
    pinMode(config::kButtonPin, INPUT_PULLDOWN_16);

    while (!display.begin()) {
        delay(2000);  // nothing to show the error on yet
    }
    display.showStatus("Sea Logger", config::kFirmwareVersion);

    gps.begin(config::kGpsBaud);
    startOrWait("SD card", [] { return logger.begin(); });

    display.showStatus("Waiting for", "GPS fix...");
}

void loop() {
    const uint32_t now = millis();

    if (button.pressed(digitalRead(config::kButtonPin) == HIGH, now)) {
        display.toggle();
    }

    if (gps.poll(reading, now)) {
        motion.update(reading.movementReported, now);

        if (tracker::shouldLog(reading.quality, motion.isMoving(), kPolicy) && logTimer.due(now)) {
            logger.add(reading.point);
        }
        if (display.isOn() && displayTimer.due(now)) {
            display.showFix(reading, motion.isMoving());
        }
        debugPrint(reading);
    }

    if (flushTimer.due(now) && !logger.flush() && config::kSerialDebug) {
        Serial.println("SD write failed, points kept for the next attempt");
    }
}
