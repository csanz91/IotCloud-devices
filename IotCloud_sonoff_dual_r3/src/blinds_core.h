#pragma once

// Platform-independent blinds logic for the Sonoff Dual R3 (no Arduino
// includes), so it can be unit tested on the host with `pio test -e native`.
//
// Time is always uint32_t milliseconds and is only ever compared through
// unsigned subtraction (now - t), which keeps working when millis() rolls over
// every ~49.7 days. Do not change it to unsigned long: that type is 64-bit on
// the host, and the rollover tests would stop testing anything.

#include <stdint.h>

// Stable-time debouncer: the accepted state only changes after the raw input
// has read the opposite value on every sample for at least `debounce_ms` AND
// at least `min_samples` consecutive samples. Any sample matching the accepted
// state cancels the pending change, so short glitches (in either direction)
// never get through. The sample count matters when loop() runs slowly: with
// 20 ms between samples, 80 ms alone would be only ~5 agreeing samples
// (Monte Carlo: 12% spurious blips at 20 ms/loop with time only, 0% with 16).
class SwitchDebouncer
{
public:
    SwitchDebouncer(uint32_t debounce_ms, uint8_t min_samples);

    void reset(bool state);
    // Feeds one raw sample. Returns true when the accepted state changes.
    bool update(bool raw, uint32_t now);
    // Drops a pending change, so the input has to be stable for a full
    // debounce window again, counted from the next sample.
    void discard_pending();
    bool state() const { return _state; }
    void set_debounce_ms(uint32_t debounce_ms) { _debounce_ms = debounce_ms; }

private:
    uint32_t _debounce_ms;
    uint8_t _min_samples;
    uint8_t _samples = 0;
    bool _state = false;
    bool _pending = false;
    uint32_t _pending_since = 0;
};

enum class BlindsDirection : uint8_t
{
    STOPPED,
    OPENING,
    CLOSING
};

struct BlindsConfig
{
    // Raw input must be stable this long before a press or release counts.
    // 80 ms is what the previous firmware used on site.
    uint32_t debounce_ms = 80;
    uint8_t debounce_min_samples = 16;
    // After any relay change, input samples are ignored for this long (the
    // motor switching is what induces the noise). A transition that happens
    // during the window is deferred, not lost: it is picked up afterwards.
    uint32_t relay_blanking_ms = 150;
    // Both relays stay off at least this long before the motor reverses.
    uint32_t reversal_dead_time_ms = 500;
    // Safety cap for a movement held from a wall switch (stuck/latched input).
    uint32_t switch_max_movement_ms = 60000;
    // Timed (remote) movements shorter than this are ignored.
    uint32_t min_movement_ms = 250;
    // Oscillation guard: more than `lockout_max_starts` switch-initiated
    // starts within `lockout_window_ms` locks the wall switches out until both
    // have read released for `lockout_release_ms`. Remote commands and stops
    // keep working during a lockout.
    uint8_t lockout_max_starts = 8;
    uint32_t lockout_window_ms = 10000;
    uint32_t lockout_release_ms = 2000;
};

class BlindsController
{
public:
    static constexpr uint32_t MIN_TIMING_MS = 1000;
    static constexpr uint32_t MAX_TIMING_MS = 60000;
    static constexpr uint8_t MAX_LOCKOUT_STARTS = 16;

    explicit BlindsController(const BlindsConfig &config = BlindsConfig());

    // Seeds the switch state from the pins. A switch that is already active
    // at boot (latched rocker) does not start the motor; it needs a new press.
    void begin(uint32_t now, bool up_raw, bool down_raw);

    // Call on every loop iteration with the raw (active = true) switch states.
    void tick(uint32_t now, bool up_raw, bool down_raw);

    // Signed percentage, -100..100: positive opens, negative closes, 0 stops.
    // Any command received while moving (or about to move) only stops.
    void command_movement(float percent, uint32_t now);
    void command_stop(uint32_t now);

    // Returns false (and leaves the value unchanged) if out of range.
    bool set_full_open_time(uint32_t ms);
    bool set_full_close_time(uint32_t ms);
    bool set_full_open_command_time(uint32_t ms);

    uint32_t full_open_time() const { return _full_open_time; }
    uint32_t full_close_time() const { return _full_close_time; }
    uint32_t full_open_command_time() const { return _full_open_command_time; }

    bool relay_up() const { return _active == BlindsDirection::OPENING; }
    bool relay_down() const { return _active == BlindsDirection::CLOSING; }
    BlindsDirection direction() const { return _active; }
    bool switch_lockout() const { return _lockout; }

    uint32_t movement_duration(BlindsDirection direction, float percent) const;

    // Parses a setMovement payload. Clamps finite values to -100..100 and
    // rejects empty, malformed, trailing-garbage and non-finite input.
    static bool parse_movement(const char *payload, float *movement);
    // Parses a timing payload: a complete integer in [MIN_TIMING_MS, MAX_TIMING_MS].
    static bool parse_timing(const char *payload, uint32_t *value);

private:
    enum class Source : uint8_t
    {
        NONE,
        REMOTE,
        SWITCH
    };

    BlindsConfig _config;
    SwitchDebouncer _up_switch;
    SwitchDebouncer _down_switch;

    uint32_t _full_open_time = 20000;
    uint32_t _full_close_time = 18333;
    uint32_t _full_open_command_time = 23500;

    // Energized relay.
    BlindsDirection _active = BlindsDirection::STOPPED;
    Source _active_source = Source::NONE;
    uint32_t _started_at = 0;
    uint32_t _duration = 0;

    // Movement waiting for the reversal dead time to expire.
    BlindsDirection _pending = BlindsDirection::STOPPED;
    Source _pending_source = Source::NONE;
    uint32_t _pending_duration = 0;

    BlindsDirection _last_direction = BlindsDirection::STOPPED;
    uint32_t _stopped_at = 0;
    uint32_t _last_relay_change = 0;
    bool _relay_changed_once = false;

    bool _lockout = false;
    bool _released_tracking = false;
    uint32_t _released_since = 0;
    uint32_t _start_times[MAX_LOCKOUT_STARTS] = {};
    uint8_t _start_count = 0;
    uint8_t _start_head = 0;

    void process_switches(uint32_t now, bool up_changed, bool down_changed);
    void switch_pressed(BlindsDirection direction, uint32_t now);
    void switch_released(BlindsDirection direction, uint32_t now);
    bool register_switch_start(uint32_t now);
    void update_lockout(uint32_t now);

    void request(BlindsDirection direction, uint32_t duration, Source source, uint32_t now);
    void energize(BlindsDirection direction, uint32_t duration, Source source, uint32_t now);
    void stop(uint32_t now);
    bool dead_time_elapsed(BlindsDirection direction, uint32_t now) const;
    bool in_blanking(uint32_t now) const;
};
