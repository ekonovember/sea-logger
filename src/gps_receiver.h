#pragma once
// u-blox NEO-6M over SoftwareSerial, parsed by NeoGPS, translated into the
// hardware-independent types of tracker_core.

#include <NMEAGPS.h>
#include <SoftwareSerial.h>

#include "track_point.h"

struct GpsReading {
    gps_fix fix;  // raw NeoGPS fix, for diagnostics

    tracker::FixQuality quality;
    tracker::TrackPoint point;     // valid only where `quality` says so
    bool hasSpeed;
    bool hasCourse;
    bool movementReported;         // heading + speed present = receiver sees motion
};

class GpsReceiver {
public:
    GpsReceiver(uint8_t rxPin, uint8_t txPin);

    void begin(uint32_t baud);

    // Non-blocking. Returns true when a complete fix (one 1 Hz burst) is ready.
    bool poll(GpsReading& out, uint32_t nowMs);

private:
    void requestErrorEstimates();

    SoftwareSerial serial_;
    NMEAGPS parser_;
};
