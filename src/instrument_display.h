#pragma once
// The 128x64 SH1106 OLED: one instrument screen plus a boot/status screen.
//
//   UTC: 14:03 01.11.2026
//   ------------------------
//   COG: 123°  | SOG: 05.3KT
//   ------------------------
//   lat:  50  15.23' N    (o)  <- accuracy in metres, or three
//   lng: 019  02.07' E         rings while logging under way

#include <SH1106Wire.h>

struct GpsReading;

class InstrumentDisplay {
public:
    InstrumentDisplay(uint8_t i2cAddress, uint8_t sdaPin, uint8_t sclPin);

    bool begin();
    void showStatus(const char* line1, const char* line2 = "");
    void showFix(const GpsReading& reading, bool underWay);

    // The display is the main power draw after the GPS; the button toggles it.
    void toggle();
    bool isOn() const { return on_; }

private:
    SH1106Wire oled_;
    bool on_ = true;
};
