#include "iotcloud_config.h"
#include <Arduino.h>
#include <iotcloud.h>

#include <presence_sensor.h>

#define SWITCH_PIN 0

Device my_device = Device("v1.0.4", "ESP32_LD2410");

void setup(void)
{
    Serial.begin(115200);
    my_device.add_sensor(new PRESENCE_SENSOR("Presence", "Presence", SWITCH_PIN));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}