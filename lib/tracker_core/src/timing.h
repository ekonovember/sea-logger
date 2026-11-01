#pragma once
// Small time-based helpers. Time is always passed in, never read from millis(),
// so the logic is deterministic and testable on the host.

#include <cstdint>

namespace tracker {

// Fires at most once per period: `due(now)` is true on the first call and then
// whenever `period` has elapsed since it last fired. Wrap-safe.
class IntervalTimer {
public:
    explicit IntervalTimer(uint32_t periodMs) : periodMs_(periodMs) {}

    bool due(uint32_t nowMs) {
        if (fired_ && nowMs - lastMs_ < periodMs_) {
            return false;
        }
        fired_ = true;
        lastMs_ = nowMs;
        return true;
    }

private:
    uint32_t periodMs_;
    uint32_t lastMs_ = 0;
    bool fired_ = false;
};

// Turns a bouncy push-button level into clean "pressed" events: a press is
// reported once, when the level has been stable-high for `settleMs`, and the
// button must be released before it can report again.
class ButtonDebouncer {
public:
    explicit ButtonDebouncer(uint32_t settleMs) : settleMs_(settleMs) {}

    // Returns true exactly once per physical press.
    bool pressed(bool levelHigh, uint32_t nowMs) {
        if (levelHigh != lastLevel_) {
            lastLevel_ = levelHigh;
            changedAtMs_ = nowMs;
            return false;
        }
        if (nowMs - changedAtMs_ < settleMs_) {
            return false;
        }
        if (levelHigh && !reported_) {
            reported_ = true;
            return true;
        }
        if (!levelHigh) {
            reported_ = false;
        }
        return false;
    }

private:
    uint32_t settleMs_;
    bool lastLevel_ = false;
    uint32_t changedAtMs_ = 0;
    bool reported_ = false;
};

}  // namespace tracker
