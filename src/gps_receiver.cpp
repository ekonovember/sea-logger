#include "gps_receiver.h"

#include "config.h"

GpsReceiver::GpsReceiver(uint8_t rxPin, uint8_t txPin) : serial_(rxPin, txPin) {}

void GpsReceiver::begin(uint32_t baud) {
    serial_.begin(baud);
    requestErrorEstimates();
}

// The NEO-6M does not emit GST (pseudorange error statistics) by default.
// PUBX,40 turns it on at 1 per fix. The setting lives in the receiver's RAM,
// so it is re-sent periodically in case the module browns out and restarts.
void GpsReceiver::requestErrorEstimates() {
    parser_.send_P(&serial_, F("PUBX,40,GST,0,1,0,0,0,0"));
}

bool GpsReceiver::poll(GpsReading& out, uint32_t nowMs) {
    if (!gstRequested_ || nowMs - lastGstRequestMs_ >= config::kGstRequestIntervalMs) {
        requestErrorEstimates();
        gstRequested_ = true;
        lastGstRequestMs_ = nowMs;
    }

    if (!parser_.available(serial_)) {
        return false;
    }

    const gps_fix fix = parser_.read();
    out.fix = fix;

    out.quality.hasLocation = fix.valid.location;
    out.quality.hasDateTime = fix.valid.date && fix.valid.time;
    out.quality.hasErrorEstimate = fix.valid.lat_err && fix.valid.lon_err;
    out.quality.latitudeErrorM = out.quality.hasErrorEstimate ? fix.lat_err() : 0.0f;
    out.quality.longitudeErrorM = out.quality.hasErrorEstimate ? fix.lon_err() : 0.0f;

    out.hasSpeed = fix.valid.speed;
    out.hasCourse = fix.valid.heading;
    out.movementReported = fix.valid.speed && fix.valid.heading;

    out.point.latitude = fix.valid.location ? fix.latitude() : 0.0;
    out.point.longitude = fix.valid.location ? fix.longitude() : 0.0;
    out.point.sogKnots = fix.valid.speed ? fix.speed() : 0.0f;
    out.point.cogDegrees = fix.valid.heading ? fix.heading() : 0.0f;
    out.point.time = tracker::UtcDateTime{
        static_cast<uint16_t>(fix.dateTime.full_year()),
        fix.dateTime.month,
        fix.dateTime.date,
        fix.dateTime.hours,
        fix.dateTime.minutes,
        fix.dateTime.seconds,
    };
    return true;
}
