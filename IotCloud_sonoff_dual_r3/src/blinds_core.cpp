#include "blinds_core.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// SwitchDebouncer

SwitchDebouncer::SwitchDebouncer(uint32_t debounce_ms, uint8_t min_samples)
    : _debounce_ms(debounce_ms), _min_samples(min_samples)
{
}

void SwitchDebouncer::reset(bool state)
{
    _state = state;
    _pending = false;
}

bool SwitchDebouncer::update(bool raw, uint32_t now)
{
    if (raw == _state)
    {
        _pending = false;
        return false;
    }

    if (!_pending)
    {
        _pending = true;
        _pending_since = now;
        _samples = 1;
        return false;
    }

    if (_samples < 255)
    {
        _samples++;
    }

    if (now - _pending_since < _debounce_ms || _samples < _min_samples)
    {
        return false;
    }

    _state = raw;
    _pending = false;
    return true;
}

void SwitchDebouncer::discard_pending()
{
    _pending = false;
}

// ---------------------------------------------------------------------------
// BlindsController

BlindsController::BlindsController(const BlindsConfig &config)
    : _config(config),
      _up_switch(config.debounce_ms, config.debounce_min_samples),
      _down_switch(config.debounce_ms, config.debounce_min_samples)
{
    if (_config.lockout_max_starts == 0)
    {
        _config.lockout_max_starts = 1;
    }
    if (_config.lockout_max_starts > MAX_LOCKOUT_STARTS)
    {
        _config.lockout_max_starts = MAX_LOCKOUT_STARTS;
    }
}

void BlindsController::begin(uint32_t now, bool up_raw, bool down_raw)
{
    (void)now;
    _up_switch.reset(up_raw);
    _down_switch.reset(down_raw);
}

void BlindsController::tick(uint32_t now, bool up_raw, bool down_raw)
{
    // Expire bookkeeping windows explicitly so a stale timestamp can never
    // look recent again after millis() rolls over.
    if (_relay_changed_once && now - _last_relay_change >= _config.relay_blanking_ms)
    {
        _relay_changed_once = false;
    }
    if (_active == BlindsDirection::STOPPED && _last_direction != BlindsDirection::STOPPED &&
        now - _stopped_at >= _config.reversal_dead_time_ms)
    {
        _last_direction = BlindsDirection::STOPPED;
    }

    // 1. Wall switches. Inputs go first so that a release seen in this tick
    //    cancels a pending start instead of letting it blip the relay.
    bool up_changed = false;
    bool down_changed = false;
    if (in_blanking(now))
    {
        _up_switch.discard_pending();
        _down_switch.discard_pending();
    }
    else
    {
        up_changed = _up_switch.update(up_raw, now);
        down_changed = _down_switch.update(down_raw, now);
    }
    process_switches(now, up_changed, down_changed);
    update_lockout(now);

    // 2. Timed auto-stop (remote movements and the switch safety cap).
    if (_active != BlindsDirection::STOPPED && now - _started_at >= _duration)
    {
        stop(now);
    }

    // 3. Start a movement that was waiting for the reversal dead time.
    if (_pending != BlindsDirection::STOPPED && dead_time_elapsed(_pending, now))
    {
        energize(_pending, _pending_duration, _pending_source, now);
    }
}

void BlindsController::process_switches(uint32_t now, bool up_changed, bool down_changed)
{
    if (!up_changed && !down_changed)
    {
        return;
    }

    const bool up = _up_switch.state();
    const bool down = _down_switch.state();

    if (up_changed && !up)
    {
        switch_released(BlindsDirection::OPENING, now);
    }
    if (down_changed && !down)
    {
        switch_released(BlindsDirection::CLOSING, now);
    }

    if (up && down)
    {
        // Contradictory input: stop whatever is moving. Neither switch starts
        // anything until it is released and pressed again.
        stop(now);
        return;
    }

    if (up_changed && up)
    {
        switch_pressed(BlindsDirection::OPENING, now);
    }
    if (down_changed && down)
    {
        switch_pressed(BlindsDirection::CLOSING, now);
    }
}

void BlindsController::switch_pressed(BlindsDirection direction, uint32_t now)
{
    if (_lockout)
    {
        return;
    }

    if (!register_switch_start(now))
    {
        _lockout = true;
        _released_tracking = false;
        if (_active_source == Source::SWITCH || _pending_source == Source::SWITCH)
        {
            stop(now);
        }
        return;
    }

    request(direction, _config.switch_max_movement_ms, Source::SWITCH, now);
}

void BlindsController::switch_released(BlindsDirection direction, uint32_t now)
{
    if (_active == direction && _active_source == Source::SWITCH)
    {
        stop(now);
    }
    else if (_pending == direction && _pending_source == Source::SWITCH)
    {
        _pending = BlindsDirection::STOPPED;
        _pending_source = Source::NONE;
    }
}

bool BlindsController::register_switch_start(uint32_t now)
{
    const uint8_t max = _config.lockout_max_starts;

    // When the ring is full, _start_head points at the oldest start. If that
    // one is still inside the window, this would be start number max + 1.
    if (_start_count == max && now - _start_times[_start_head] < _config.lockout_window_ms)
    {
        _start_count = 0;
        _start_head = 0;
        return false;
    }

    _start_times[_start_head] = now;
    _start_head = static_cast<uint8_t>((_start_head + 1) % max);
    if (_start_count < max)
    {
        _start_count++;
    }
    return true;
}

void BlindsController::update_lockout(uint32_t now)
{
    if (!_lockout)
    {
        return;
    }

    if (_up_switch.state() || _down_switch.state())
    {
        _released_tracking = false;
        return;
    }

    if (!_released_tracking)
    {
        _released_tracking = true;
        _released_since = now;
        return;
    }

    if (now - _released_since >= _config.lockout_release_ms)
    {
        _lockout = false;
        _released_tracking = false;
    }
}

void BlindsController::command_movement(float percent, uint32_t now)
{
    if (!isfinite(percent))
    {
        return;
    }

    if (percent == 0.0f || _active != BlindsDirection::STOPPED || _pending != BlindsDirection::STOPPED)
    {
        command_stop(now);
        return;
    }

    const BlindsDirection direction = percent > 0.0f ? BlindsDirection::OPENING : BlindsDirection::CLOSING;
    const uint32_t duration = movement_duration(direction, fabsf(percent));
    if (duration < _config.min_movement_ms)
    {
        return;
    }

    request(direction, duration, Source::REMOTE, now);
}

void BlindsController::command_stop(uint32_t now)
{
    stop(now);
}

static bool timing_in_range(uint32_t ms)
{
    return ms >= BlindsController::MIN_TIMING_MS && ms <= BlindsController::MAX_TIMING_MS;
}

bool BlindsController::set_full_open_time(uint32_t ms)
{
    if (!timing_in_range(ms))
    {
        return false;
    }
    _full_open_time = ms;
    return true;
}

bool BlindsController::set_full_close_time(uint32_t ms)
{
    if (!timing_in_range(ms))
    {
        return false;
    }
    _full_close_time = ms;
    return true;
}

bool BlindsController::set_full_open_command_time(uint32_t ms)
{
    if (!timing_in_range(ms))
    {
        return false;
    }
    _full_open_command_time = ms;
    return true;
}

uint32_t BlindsController::movement_duration(BlindsDirection direction, float percent) const
{
    if (direction == BlindsDirection::STOPPED || !(percent > 0.0f))
    {
        return 0;
    }

    if (percent >= 100.0f)
    {
        return direction == BlindsDirection::OPENING ? _full_open_command_time : _full_close_time;
    }

    const uint32_t full_time = direction == BlindsDirection::OPENING ? _full_open_time : _full_close_time;
    return static_cast<uint32_t>(full_time * percent / 100.0f);
}

void BlindsController::request(BlindsDirection direction, uint32_t duration, Source source, uint32_t now)
{
    if (_active == direction)
    {
        // Already moving this way: the new source takes over (e.g. holding the
        // wall switch during a remote movement keeps it going until release).
        _active_source = source;
        _started_at = now;
        _duration = duration;
        _pending = BlindsDirection::STOPPED;
        _pending_source = Source::NONE;
        return;
    }

    stop(now);

    if (dead_time_elapsed(direction, now))
    {
        energize(direction, duration, source, now);
    }
    else
    {
        _pending = direction;
        _pending_source = source;
        _pending_duration = duration;
    }
}

void BlindsController::energize(BlindsDirection direction, uint32_t duration, Source source, uint32_t now)
{
    _pending = BlindsDirection::STOPPED;
    _pending_source = Source::NONE;

    _active = direction;
    _active_source = source;
    _started_at = now;
    _duration = duration;

    _last_relay_change = now;
    _relay_changed_once = true;
}

void BlindsController::stop(uint32_t now)
{
    _pending = BlindsDirection::STOPPED;
    _pending_source = Source::NONE;

    if (_active == BlindsDirection::STOPPED)
    {
        return;
    }

    _last_direction = _active;
    _stopped_at = now;
    _active = BlindsDirection::STOPPED;
    _active_source = Source::NONE;
    _duration = 0;

    _last_relay_change = now;
    _relay_changed_once = true;
}

bool BlindsController::dead_time_elapsed(BlindsDirection direction, uint32_t now) const
{
    if (_last_direction == BlindsDirection::STOPPED || _last_direction == direction)
    {
        return true;
    }
    return now - _stopped_at >= _config.reversal_dead_time_ms;
}

bool BlindsController::in_blanking(uint32_t now) const
{
    return _relay_changed_once && now - _last_relay_change < _config.relay_blanking_ms;
}

// ---------------------------------------------------------------------------
// Payload parsing

static const char *skip_space(const char *p)
{
    while (*p != '\0' && isspace(static_cast<unsigned char>(*p)))
    {
        p++;
    }
    return p;
}

bool BlindsController::parse_movement(const char *payload, float *movement)
{
    if (payload == nullptr || movement == nullptr)
    {
        return false;
    }

    const char *start = skip_space(payload);
    if (*start == '\0')
    {
        return false;
    }

    char *end = nullptr;
    float parsed = strtof(start, &end);
    if (end == start || *skip_space(end) != '\0' || !isfinite(parsed))
    {
        return false;
    }

    if (parsed > 100.0f)
    {
        parsed = 100.0f;
    }
    else if (parsed < -100.0f)
    {
        parsed = -100.0f;
    }

    *movement = parsed;
    return true;
}

bool BlindsController::parse_timing(const char *payload, uint32_t *value)
{
    if (payload == nullptr || value == nullptr)
    {
        return false;
    }

    const char *start = skip_space(payload);
    if (*start == '\0')
    {
        return false;
    }

    errno = 0;
    char *end = nullptr;
    long parsed = strtol(start, &end, 10);
    if (errno == ERANGE || end == start || *skip_space(end) != '\0' ||
        parsed < static_cast<long>(MIN_TIMING_MS) || parsed > static_cast<long>(MAX_TIMING_MS))
    {
        return false;
    }

    *value = static_cast<uint32_t>(parsed);
    return true;
}
