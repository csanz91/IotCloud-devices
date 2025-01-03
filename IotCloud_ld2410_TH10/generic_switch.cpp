#include "generic_switch.h"

GENERIC_TOOGLE::GENERIC_TOOGLE(
    const char *sensor_id,
    const char *sensor_name,
    const int relay_pin,
    const int switch_pin) : SwitchSensor(sensor_id,
                                         sensor_name,
                                         "switch",
                                         "{}"), // 1% filtering
                            _relay_pin(relay_pin),
                            _switch_pin(switch_pin)
{

    pinMode(_relay_pin, OUTPUT);
    digitalWrite(_relay_pin, LOW);

    pinMode(_switch_pin, INPUT_PULLUP);

    _last_state = digitalRead(_switch_pin);

    _setup_time = millis();
}

void GENERIC_TOOGLE::set_state(const bool state)
{
    SwitchSensor::set_state(state);
    digitalWrite(_relay_pin, state);
}

void GENERIC_TOOGLE::loop()
{

    const unsigned long now = millis();

    // Ignore the inputs during the first seconds. Avoid false triggers
    if (now - _setup_time < START_DELAY){
        return;
    }

    // Debounce the switch
    bool switch_state = digitalRead(_switch_pin);
    if (switch_state != _last_state)
    {
        _mem_switch_state_1 = true;
        _last_debounce_time_1 = now;
    }

    if ((now - _last_debounce_time_1) > 50)
    {
        if (_mem_switch_state_1)
        {
            _mem_switch_state_1 = false;
            GENERIC_TOOGLE::set_state(switch_state);
        }
    }

    _last_state = switch_state;
}