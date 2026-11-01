#pragma once
// Every tunable of the tracker in one place.

#include <Arduino.h>

namespace config {

// Shown on the start-up screen; matches the git tag.
constexpr const char* kFirmwareVersion = "v2.0";

// --- Wiring (NodeMCU v3 pin labels) -----------------------------------------
constexpr uint8_t kGpsRxPin = D3;      // NodeMCU RX  <- NEO-6M TX
constexpr uint8_t kGpsTxPin = D4;      // NodeMCU TX  -> NEO-6M RX
constexpr uint8_t kSdChipSelectPin = D8;
constexpr uint8_t kButtonPin = D0;     // to 3V3 through the button, 10k pull-down
constexpr uint8_t kDisplayI2cAddress = 0x3C;  // SH1106 on D2 (SDA) / D1 (SCL)

constexpr uint32_t kGpsBaud = 9600;     // NEO-6M factory default
constexpr uint32_t kSerialBaud = 115200;

// --- Logging policy -----------------------------------------------------------
// Log only while under way. Set to false to log every accurate fix, e.g. when
// testing at a desk.
constexpr bool kRequireMovement = true;
// Mean of the receiver's lat/lon 1-sigma error (GST sentence) must be <= this.
constexpr float kMaxPositionErrorM = 15.0f;
// Movement must be reported continuously this long to start / stop logging.
constexpr uint32_t kMotionStartMs = 6000;
constexpr uint32_t kMotionStopMs = 3000;

// --- Timing ---------------------------------------------------------------------
constexpr uint32_t kDisplayRefreshMs = 2000;
constexpr uint32_t kLogIntervalMs = 3000;      // one track point every 3 s
constexpr uint32_t kSdFlushIntervalMs = 60000; // write the buffer once a minute
constexpr uint32_t kButtonSettleMs = 30;

// RAM buffer between SD writes. A minute at one point per 3 s is 20 points;
// 32 leaves headroom for a slow card. 32 x 32 B = 1 kB, no heap involved.
constexpr size_t kTrackBufferCapacity = 32;

// --- Diagnostics -------------------------------------------------------------------
// Print every fix and every SD write to the serial monitor.
constexpr bool kSerialDebug = false;

}  // namespace config
