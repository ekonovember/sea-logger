#pragma once
// Fixed-capacity FIFO with no heap allocation.
//
// Positions are buffered in RAM and written to the SD card in batches (one
// file open per minute instead of one per fix). On an ESP8266 with ~40 kB of
// free heap a growing linked list fragments memory; this buffer is sized at
// compile time and, when full, drops the oldest point rather than failing.

#include <cstddef>

namespace tracker {

template <typename T, size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 0, "RingBuffer needs a non-zero capacity");

public:
    // Returns false when the buffer was full and the oldest item was overwritten.
    bool push(const T& item) {
        const bool hadRoom = count_ < Capacity;
        items_[(head_ + count_) % Capacity] = item;
        if (hadRoom) {
            ++count_;
        } else {
            head_ = (head_ + 1) % Capacity;
        }
        return hadRoom;
    }

    // i = 0 is the oldest item.
    const T& at(size_t i) const { return items_[(head_ + i) % Capacity]; }

    size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == Capacity; }
    static constexpr size_t capacity() { return Capacity; }
    void clear() {
        head_ = 0;
        count_ = 0;
    }

private:
    T items_[Capacity]{};
    size_t head_ = 0;
    size_t count_ = 0;
};

}  // namespace tracker
