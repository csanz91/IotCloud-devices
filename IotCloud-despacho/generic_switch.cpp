#include "generic_switch.h"

GENERIC_TOOGLE::GENERIC_TOOGLE(
  const char *sensor_id,
  const char *sensor_name,
  const int switch_pin)
  : SwitchSensor(sensor_id,
                 sensor_name,
                 "switch",
                 "{}"),
    _switch_pin(switch_pin) {


  pinMode(_switch_pin, INPUT_PULLUP);

  _last_state = digitalRead(_switch_pin);

  _setup_time = millis();
}


void GENERIC_TOOGLE::loop() {
  const unsigned long now = millis();

  // Ignore the inputs during the first seconds. Avoid false triggers
  if (now - _setup_time < START_DELAY) {
    return;
  }

  // Debounce the switch
  bool switch_state = digitalRead(_switch_pin);
  if (switch_state != _last_state) {
    _mem_switch_state_1 = true;
    _last_debounce_time_1 = now;
  }

  if ((now - _last_debounce_time_1) > 50) {
    if (_mem_switch_state_1) {
      _mem_switch_state_1 = false;
      report_presence(switch_state);
    }
  }

  _last_state = switch_state;
}


void GENERIC_TOOGLE::report_presence(bool presence) {
  if (!_mqtt_client->isConnected()) {
    return;
  }
  char constructedTopic[110] = "";
  construct_topic(constructedTopic, "aux/presence");
  _mqtt_client->publish(constructedTopic, presence ? "true" : "false", true);
}