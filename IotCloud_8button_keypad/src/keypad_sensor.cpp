#include "keypad_sensor.h"

KeypadSensor::KeypadSensor(const char *sensor_id, const char *sensor_name)
    : BaseSensor(
          sensor_id,
          sensor_name,
          "analog_subsensor",
          "{}") {}

void KeypadSensor::publish_key(uint8_t key) {
  publish_event("aux/key", key);
}

void KeypadSensor::publish_long_key(uint8_t key) {
  publish_event("aux/keyLong", key);
}

void KeypadSensor::publish_event(const char *endpoint, uint8_t key) {
  if (_mqtt_client == nullptr || !_mqtt_client->isConnected()) {
    return;
  }

  char constructedTopic[128] = "";
  char payload[4] = "";
  construct_topic(constructedTopic, sizeof(constructedTopic), endpoint);
  snprintf(payload, sizeof(payload), "%u", key);
  _mqtt_client->publish(constructedTopic, payload, false);
}
