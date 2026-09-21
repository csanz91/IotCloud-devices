#pragma once

#include <sensors/switch.h>

class GENERIC_TOOGLE : public SwitchSensor
{
private:
    const int _relay_pin;
    const int _switch_pin;

    bool _switch_raw_state = false;
    bool _switch_stable_state = false;
    unsigned long _switch_changed_at = 0;

    bool _input_initialized = false;
    unsigned long _setup_time = 0;
    static constexpr unsigned long START_DELAY = 5000;
    static constexpr unsigned long DEBOUNCE_DELAY = 50;

    void set_state(bool) override;
    void loop() override;
    void initialize_input();

public:
    GENERIC_TOOGLE(
        const char *sensor_id,
        const char *sensor_name,
        int relay_pin,
        int switch_pin);
};
