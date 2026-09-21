#include "generic_switch.h"

GENERIC_TOOGLE::GENERIC_TOOGLE(
    const char *sensor_id,
    const char *sensor_name,
    const int relay_pin,
    const int switch_pin) : SwitchSensor(sensor_id,
                                          sensor_name,
                                          "switch",
                                          "{}"),
                             _relay_pin(relay_pin),
                             _switch_pin(switch_pin)
{
    pinMode(_relay_pin, OUTPUT);
    digitalWrite(_relay_pin, LOW);

    // GPIO5 is driven by the Shelly SW input circuit.
    pinMode(_switch_pin, INPUT);

    _setup_time = millis();
}

void GENERIC_TOOGLE::set_state(const bool new_state)
{
    SwitchSensor::set_state(new_state);
    digitalWrite(_relay_pin, new_state ? HIGH : LOW);
}

void GENERIC_TOOGLE::loop()
{
    const unsigned long now = millis();

    // Ignore the input during startup to avoid false relay toggles.
    if (now - _setup_time < START_DELAY)
    {
        return;
    }

    // Establish a baseline without treating the switch position at boot as
    // an edge. The input is a sticky switch, so either edge toggles the relay.
    if (!_input_initialized)
    {
        initialize_input();
        return;
    }

    const bool reading = digitalRead(_switch_pin);

    if (reading != _switch_raw_state)
    {
        _switch_raw_state = reading;
        _switch_changed_at = now;
    }

    if (reading != _switch_stable_state &&
        now - _switch_changed_at >= DEBOUNCE_DELAY)
    {
        _switch_stable_state = reading;
        set_state(!state);
    }
}

void GENERIC_TOOGLE::initialize_input()
{
    _switch_raw_state = digitalRead(_switch_pin);
    _switch_stable_state = _switch_raw_state;
    _input_initialized = true;
}
