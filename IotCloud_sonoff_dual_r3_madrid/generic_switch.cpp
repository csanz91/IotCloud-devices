#include "generic_switch.h"

GENERIC_TOOGLE::GENERIC_TOOGLE(
  const char *sensor_id,
  const char *sensor_name,
  const int relay_pin,
  const int switch_pin)
  : SwitchSensor(sensor_id,
                 sensor_name,
                 "switch",
                 "{}"),
    _relay_pin(relay_pin),
    _switch_pin(switch_pin) {

  pinMode(_relay_pin, OUTPUT);
  digitalWrite(_relay_pin, LOW);

  pinMode(_switch_pin, INPUT_PULLUP);

}

void GENERIC_TOOGLE::set_state(const bool state)
{
    SwitchSensor::set_state(state);
    digitalWrite(_relay_pin, state);
}

void GENERIC_TOOGLE::loop()
{
    const unsigned long DEBOUNCE_DELAY = 50;
    
    bool reading = digitalRead(_switch_pin);

    if (reading != _last_state && !_mem_switch_state_1)
    {
        _initial_reading = reading;
        _last_debounce_time_1 = millis();
        _mem_switch_state_1 = true;
    }
    else if (_mem_switch_state_1 && (millis() - _last_debounce_time_1) > DEBOUNCE_DELAY)
    {
        if (reading == _initial_reading)
        {
            GENERIC_TOOGLE::set_state(!state);
        }
        _mem_switch_state_1 = false;
    }

    _last_state = reading;
}