#pragma once

#include <Arduino.h>
#include <sensors/sensor.h>

class KeypadSensor : public BaseSensor {
private:
  void publish_event(const char *endpoint, uint8_t key);

public:
  KeypadSensor(const char *sensor_id, const char *sensor_name);
  void publish_key(uint8_t key);
  void publish_long_key(uint8_t key);
};
