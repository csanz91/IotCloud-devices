#pragma once

#include <sensors/switch.h>

class GENERIC_TOOGLE : public SwitchSensor
{
private:
    const int _relay_pin;
    const int _manual_switch_pin;

    bool _manual_raw_state;
    bool _manual_stable_state;
    unsigned long _manual_changed_at = 0;

    bool _inputs_initialized = false;
    unsigned long _setup_time = 0;
    const unsigned long START_DELAY = 5000;
    const unsigned long DEBOUNCE_DELAY = 50;

    void set_state(bool) override;
    void loop() override;
    void initialize_inputs();
    void update_manual_switch(unsigned long now);

public:
    GENERIC_TOOGLE(
        const char *sensor_id,
        const char *sensor_name,
        const int relay_pin,
        const int manual_switch_pin);
};
