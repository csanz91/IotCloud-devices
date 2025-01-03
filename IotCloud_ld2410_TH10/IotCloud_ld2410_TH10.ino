#include <Arduino.h>
#include <iotcloud.h>
#include "generic_switch.h"

#define RELAY_PIN 12
#define SWITCH_PIN 2

Device my_device = Device("v1.0.0", "ESP8266_LD2410_TH10");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new GENERIC_TOOGLE("KitchenLight", "Switch", RELAY_PIN, SWITCH_PIN));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}