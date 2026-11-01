#pragma once
// Decides whether the vessel is under way, with hysteresis.
//
// A GPS reports a heading only while it sees movement, but a single noisy fix
// (or a gust swinging a moored boat) must not start a log. Movement has to be
// reported continuously for `startAfterMs` to count as under way, and missing
// continuously for `stopAfterMs` to count as stopped again.

#include <cstdint>

namespace tracker {

class MotionDetector {
public:
    MotionDetector(uint32_t startAfterMs, uint32_t stopAfterMs)
        : startAfterMs_(startAfterMs), stopAfterMs_(stopAfterMs) {}

    // Call for every fix: `movementReported` = the fix carries a valid heading and speed.
    void update(bool movementReported, uint32_t nowMs) {
        if (movementReported == moving_) {
            pending_ = false;  // consistent with the current state, nothing to change
            return;
        }
        if (!pending_) {
            pending_ = true;
            pendingSinceMs_ = nowMs;
        }
        const uint32_t needed = moving_ ? stopAfterMs_ : startAfterMs_;
        // Unsigned subtraction stays correct when millis() wraps after ~49 days.
        if (nowMs - pendingSinceMs_ >= needed) {
            moving_ = !moving_;
            pending_ = false;
        }
    }

    bool isMoving() const { return moving_; }

private:
    uint32_t startAfterMs_;
    uint32_t stopAfterMs_;
    bool moving_ = false;
    bool pending_ = false;
    uint32_t pendingSinceMs_ = 0;
};

}  // namespace tracker
