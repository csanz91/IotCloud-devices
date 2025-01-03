#pragma once
#include <sensors/toogle.h>

class GENERIC_TOOGLE : public Toogle
{
private:
    const int _relay1_pin;
    const int _relay2_pin;
    const int _switch1_pin;
    const int _switch2_pin;
    void set_relay();
    bool _last_state_1;
    bool _last_state_2;
    bool _mem_switch_state_1 = false;
    bool _mem_switch_state_2 = false;
    bool _mem_switch_deact_state_1 = false;
    bool _mem_switch_deact_state_2 = false;
    bool _switch_activated_1 = false;
    bool _switch_activated_2 = false;
    unsigned long _last_debounce_time_1 = 0;
    unsigned long _last_debounce_time_2 = 0;
    unsigned long _last_debounce_deact_time_1 = 0;
    unsigned long _last_debounce_deact_time_2 = 0;
    const int DEBOUNCE_TIME = 80;
    void loop();

public:
    GENERIC_TOOGLE(
        const char *sensor_id,
        const char *sensor_name,
        const int relay1_pin,
        const int relay2_pin,
        const int switch1_pin,
        const int switch2_pin);
};