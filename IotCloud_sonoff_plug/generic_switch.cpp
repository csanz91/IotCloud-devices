#include "generic_switch.h"

GENERIC_SWITCH::GENERIC_SWITCH(
  const char *sensor_id,
  const char *sensor_name,
  const int relay_pin,
  const int switch_pin)
  : SwitchSensor(sensor_id,
                 sensor_name,
                 "switch",
                 "{}"),  // 1% filtering
    _relay_pin(relay_pin),
    _switch_pin(switch_pin) {

  pinMode(_relay_pin, OUTPUT);
  digitalWrite(_relay_pin, LOW);

  pinMode(_switch_pin, INPUT_PULLUP);

  _last_state = digitalRead(_switch_pin);
}

void GENERIC_SWITCH::set_state(const bool state) {
  SwitchSensor::set_state(state);
  digitalWrite(_relay_pin, state);
}

void GENERIC_SWITCH::loop() {
  // Debounce the switch
  const unsigned long DEBOUNCE_DELAY = 50;

  bool reading = digitalRead(_switch_pin);

  if (reading != _last_state) {
    _last_debounce_time_1 = millis();
  }

  if (!reading && !_mem_switch_state_1) {
    GENERIC_SWITCH::set_state(!state);
    _mem_switch_state_1 = true;
  }

  if (_mem_switch_state_1 && (millis() - _last_debounce_time_1) > DEBOUNCE_DELAY) {
    if (reading) {
      _mem_switch_state_1 = false;
    }
  }
  _last_state = reading;
}