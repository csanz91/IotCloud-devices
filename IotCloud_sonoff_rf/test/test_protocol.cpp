#include "../src/rf_protocol.h"
#include <cassert>
#include <cstdio>
#include <vector>

static std::vector<uint8_t> packet(uint32_t code = 0xAA5580) {
    return {0xAA, 0xA4, 0x20, 0x55, 0x01, 0xAA, 0x03, 0x80,
            uint8_t(code >> 16), uint8_t(code >> 8), uint8_t(code), 0x55};
}
int main() {
    rf::Parser parser;
    rf::Frame frame;
    uint32_t now = 0;
    int frames = 0;
    for (auto byte : packet()) if (parser.feed(byte, now++, frame)) ++frames;
    assert(frames == 1 && frame.action == 0xA4);
    assert(frame.signal.code == 0xAA5580 && frame.signal.sync == 0x2055);
    assert(frame.signal.low == 0x01AA && frame.signal.high == 0x0380);
    // Noise, unsupported actions, malformed trailers, back-to-back frames.
    for (auto byte : {0x55, 0, 0xAA, 0xFF}) assert(!parser.feed(byte, now++, frame));
    auto broken = packet(); broken.back() = 0;
    for (auto byte : broken) assert(!parser.feed(byte, now++, frame));
    frames = 0;
    for (int i = 0; i < 2; ++i)
        for (auto byte : packet(1906529)) if (parser.feed(byte, now++, frame)) ++frames;
    assert(frames == 2 && frame.signal.code == 1906529);
    // Truncation followed by timeout and recovery.
    assert(!parser.feed(0xAA, now++, frame));
    assert(!parser.feed(0xA4, now++, frame));
    now += 101;
    frames = 0;
    for (auto byte : packet()) if (parser.feed(byte, now++, frame)) ++frames;
    assert(frames == 1 && parser.errors > 0);
    for (uint8_t action : {0xA0, 0xA1, 0xA2}) {
        assert(!parser.feed(0xAA, now++, frame));
        assert(!parser.feed(action, now++, frame));
        assert(parser.feed(0x55, now++, frame) && frame.action == action);
    }
    // No out-of-bounds access under long random streams (run with sanitizers).
    uint32_t random = 123;
    for (int i = 0; i < 100000; ++i) {
        random = random * 1664525 + 1013904223;
        parser.feed(random >> 24, now++, frame);
    }
    rf::Signal reference; reference.sync = 8000; reference.low = 300;
    reference.high = 900; reference.code = 123;
    rf::Signal value = reference;
    value.low = 375; assert(rf::matches(value, reference));
    value.low = 376; assert(!rf::matches(value, reference));
    value = reference; ++value.code; assert(!rf::matches(value, reference));
    value = reference; value.sync = 0; assert(!value.valid());
    rf::Filter door, other;
    assert(!door.accept(0, 2, 3000));
    assert(door.accept(100, 2, 3000));
    assert(other.accept(101, 1, 3000)); // independent device
    assert(!door.accept(200, 2, 3000));
    assert(!door.accept(300, 2, 3000));
    assert(!door.accept(3100, 2, 3000));
    assert(door.accept(3200, 2, 3000));
    rf::Filter sparse;
    assert(!sparse.accept(0, 2, 100));
    assert(!sparse.accept(501, 2, 100));
    assert(sparse.accept(600, 2, 100));
    rf::Filter wrap;
    assert(wrap.accept(UINT32_MAX - 100, 1, 200));
    assert(!wrap.accept(50, 1, 200));
    assert(wrap.accept(100, 1, 200));
    puts("RF protocol/filter tests passed");
}
