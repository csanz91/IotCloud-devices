#include <Arduino.h>
#include <iotcloud.h>
#include "generic_switch.h"

#define RELAY_PIN D2
#define SWITCH_PIN D1

Device my_device = Device("v1.0.1", "ESP8266_LD2410");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new GENERIC_TOOGLE("BathroomLight", "Switch", RELAY_PIN, SWITCH_PIN));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}