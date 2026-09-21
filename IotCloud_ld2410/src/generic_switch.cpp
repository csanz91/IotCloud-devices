#include "generic_switch.h"

GENERIC_TOOGLE::GENERIC_TOOGLE(
    const char *sensor_id,
    const char *sensor_name,
    const int relay_pin,
    const int presence_pin,
    const int manual_switch_pin) : SwitchSensor(sensor_id,
                                                sensor_name,
                                                "switch",
                                                "{}"),
                                   _relay_pin(relay_pin),
                                   _presence_pin(presence_pin),
                                   _manual_switch_pin(manual_switch_pin)
{
    pinMode(_relay_pin, OUTPUT);
    digitalWrite(_relay_pin, LOW);

    pinMode(_presence_pin, INPUT_PULLUP);
    pinMode(_manual_switch_pin, INPUT_PULLUP);

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

    // Ignore the inputs during the first seconds. Avoid false triggers
    if (now - _setup_time < START_DELAY)
    {
        return;
    }

    // Capture both current input levels after startup without treating either
    // one as an edge. In particular, the manual switch position must not
    // toggle the relay when the device boots.
    if (!_inputs_initialized)
    {
        initialize_inputs();
        return;
    }

    update_presence(now);
    update_manual_switch(now);
}

void GENERIC_TOOGLE::initialize_inputs()
{
    _presence_raw_state = digitalRead(_presence_pin);
    _presence_stable_state = _presence_raw_state;
    _manual_raw_state = digitalRead(_manual_switch_pin);
    _manual_stable_state = _manual_raw_state;
    _inputs_initialized = true;
}

void GENERIC_TOOGLE::update_presence(const unsigned long now)
{
    const bool reading = digitalRead(_presence_pin);

    if (reading != _presence_raw_state)
    {
        _presence_raw_state = reading;
        _presence_changed_at = now;
    }

    if (reading != _presence_stable_state && now - _presence_changed_at >= DEBOUNCE_DELAY)
    {
        _presence_stable_state = reading;
        set_state(reading);
    }
}

void GENERIC_TOOGLE::update_manual_switch(const unsigned long now)
{
    const bool reading = digitalRead(_manual_switch_pin);

    if (reading != _manual_raw_state)
    {
        _manual_raw_state = reading;
        _manual_changed_at = now;
    }

    if (reading != _manual_stable_state && now - _manual_changed_at >= DEBOUNCE_DELAY)
    {
        _manual_stable_state = reading;
        set_state(!state);
    }
}
