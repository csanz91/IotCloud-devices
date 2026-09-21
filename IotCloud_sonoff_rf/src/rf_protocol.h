#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace rf {
struct Signal {
    uint16_t sync = 0, low = 0, high = 0;
    uint32_t code = 0;
    bool valid() const { return sync && low && high && code <= 0xFFFFFF; }
};
inline bool near(uint16_t value, uint16_t reference, uint8_t tolerance = 25) {
    const uint32_t delta = value > reference ? value - reference : reference - value;
    return reference && delta * 100 <= uint32_t(reference) * tolerance;
}
inline bool matches(const Signal &value, const Signal &reference) {
    return value.code == reference.code && near(value.sync, reference.sync) &&
           near(value.low, reference.low) && near(value.high, reference.high);
}
struct Frame { uint8_t action = 0; Signal signal; };

// Stock iTead UART: AA action [9-byte payload for A3/A4/A5] 55.
// Payload markers are data. Sliding a bounded buffer recovers after corruption.
class Parser {
    uint8_t bytes_[12] = {};
    size_t size_ = 0;
    uint32_t last_ = 0;
    void drop() { --size_; memmove(bytes_, bytes_ + 1, size_); }
public:
    uint32_t errors = 0;
    bool feed(uint8_t byte, uint32_t now, Frame &frame) {
        if (size_ && uint32_t(now - last_) > 100) { size_ = 0; ++errors; }
        last_ = now;
        bytes_[size_++] = byte;
        while (size_) {
            if (bytes_[0] != 0xAA) { drop(); continue; }
            if (size_ < 2) return false;
            const uint8_t action = bytes_[1];
            if (action < 0xA0 || action > 0xA5) { ++errors; drop(); continue; }
            const size_t expected = action >= 0xA3 ? 12 : 3;
            if (size_ < expected) return false;
            if (bytes_[expected - 1] != 0x55) { ++errors; drop(); continue; }
            frame = Frame{};
            frame.action = action;
            if (expected == 12) {
                frame.signal.sync = uint16_t(bytes_[2]) << 8 | bytes_[3];
                frame.signal.low = uint16_t(bytes_[4]) << 8 | bytes_[5];
                frame.signal.high = uint16_t(bytes_[6]) << 8 | bytes_[7];
                frame.signal.code = uint32_t(bytes_[8]) << 16 | uint32_t(bytes_[9]) << 8 | bytes_[10];
            }
            size_ -= expected;
            memmove(bytes_, bytes_ + expected, size_);
            return true;
        }
        return false;
    }
};
struct Filter {
    uint32_t first = 0, last = 0, emitted = 0;
    uint8_t count = 0;
    bool hasEmitted = false;
    bool accept(uint32_t now, uint8_t required, uint32_t debounce) {
        if (!count || uint32_t(now - first) > 1000 || uint32_t(now - last) > 500) {
            first = now; count = 0;
        }
        last = now;
        if (count < 255) ++count;
        if (count < required || (hasEmitted && uint32_t(now - emitted) < debounce)) return false;
        emitted = now; hasEmitted = true; count = 0;
        return true;
    }
};
} // namespace rf
