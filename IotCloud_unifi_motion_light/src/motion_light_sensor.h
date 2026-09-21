#pragma once

#include <cstdint>

#include <sensors/switch.h>

#include "motion_light_state.h"

class MotionLightSensor : public SwitchSensor
{
public:
    MotionLightSensor(
        const char *sensor_id,
        const char *sensor_name,
        uint8_t relay_pin,
        uint8_t wall_switch_pin,
        bool relay_active_high);

    void init(char *mqtt_header, EspMQTTClient *mqtt_client) override;
    void loop() override;
    void trigger_motion(uint32_t now_ms);

private:
    static constexpr uint32_t WALL_SWITCH_DEBOUNCE_MS = 50;

    const uint8_t _relay_pin;
    const uint8_t _wall_switch_pin;
    const bool _relay_active_high;
    MotionLightState _controller;

    bool _wall_initialized = false;
    bool _wall_raw_state = false;
    bool _wall_stable_state = false;
    uint32_t _wall_changed_at_ms = 0;

    void set_state(bool new_state) override;
    void apply_output(bool force_report = false);
    void update_wall_switch(uint32_t now_ms);
    void subscribe_timeout_setting(EspMQTTClient *mqtt_client);
};

