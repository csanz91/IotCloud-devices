#include "motion_light_state.h"

bool MotionLightState::trigger_motion(const uint32_t now_ms)
{
    if (_mode == Mode::MANUAL_ON)
    {
        return false;
    }

    const bool output_changed = _mode == Mode::OFF;
    _mode = Mode::MOTION_ON;
    _motion_started_at_ms = now_ms;
    _active_timeout_ms = _configured_timeout_ms;
    return output_changed;
}

bool MotionLightState::manual_set(const bool on)
{
    const bool was_on = is_on();
    _mode = on ? Mode::MANUAL_ON : Mode::OFF;
    _active_timeout_ms = 0;
    return was_on != on;
}

bool MotionLightState::manual_toggle()
{
    return manual_set(!is_on());
}

bool MotionLightState::tick(const uint32_t now_ms)
{
    if (_mode != Mode::MOTION_ON ||
        now_ms - _motion_started_at_ms < _active_timeout_ms)
    {
        return false;
    }

    _mode = Mode::OFF;
    _active_timeout_ms = 0;
    return true;
}

bool MotionLightState::set_timeout_seconds(const uint32_t timeout_seconds)
{
    if (timeout_seconds < MIN_TIMEOUT_SECONDS ||
        timeout_seconds > MAX_TIMEOUT_SECONDS)
    {
        return false;
    }

    _configured_timeout_ms = timeout_seconds * MILLISECONDS_PER_SECOND;
    return true;
}

bool MotionLightState::is_on() const
{
    return _mode != Mode::OFF;
}

bool MotionLightState::motion_timer_active() const
{
    return _mode == Mode::MOTION_ON;
}

MotionLightState::Mode MotionLightState::mode() const
{
    return _mode;
}

uint32_t MotionLightState::timeout_seconds() const
{
    return _configured_timeout_ms / MILLISECONDS_PER_SECOND;
}

