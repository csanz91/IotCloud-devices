#pragma once
#include <sensors/switch.h>

class GENERIC_SWITCH : public SwitchSensor
{
private:
    const int _relay_pin;
    const int _switch_pin;
    void set_state(bool);
    bool _last_state;
    bool _mem_switch_state = false;
    unsigned long _last_debounce_time = 0;
    void loop();

public:
    GENERIC_SWITCH(
        const char *sensor_id,
        const char *sensor_name,
        const int relay_pin,
        const int switch_pin);
};