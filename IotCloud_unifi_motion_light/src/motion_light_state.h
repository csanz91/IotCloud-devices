#pragma once

#include <cstdint>

class MotionLightState
{
public:
    enum class Mode : uint8_t
    {
        OFF,
        MOTION_ON,
        MANUAL_ON,
    };

    static constexpr uint32_t DEFAULT_TIMEOUT_SECONDS = 30;
    static constexpr uint32_t MIN_TIMEOUT_SECONDS = 10;
    static constexpr uint32_t MAX_TIMEOUT_SECONDS = 1800;

    bool trigger_motion(uint32_t now_ms);
    bool manual_set(bool on);
    bool manual_toggle();
    bool tick(uint32_t now_ms);
    bool set_timeout_seconds(uint32_t timeout_seconds);

    bool is_on() const;
    bool motion_timer_active() const;
    Mode mode() const;
    uint32_t timeout_seconds() const;

private:
    static constexpr uint32_t MILLISECONDS_PER_SECOND = 1000;

    Mode _mode = Mode::OFF;
    uint32_t _configured_timeout_ms = DEFAULT_TIMEOUT_SECONDS * MILLISECONDS_PER_SECOND;
    uint32_t _active_timeout_ms = 0;
    uint32_t _motion_started_at_ms = 0;
};

