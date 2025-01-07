#include <Arduino.h>
#include <iotcloud.h>
#include "generic_switch.h"

#define RELAY_PIN 12
#define SWITCH_PIN 0

// Al cyphers
// Generic ESP8285
// Builtin led 13
// no dtr, aka ck
// 1MB FS none

Device my_device = Device("v1.0.6", "SONOFF_PLUG");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new GENERIC_SWITCH("SonoffPlug", "Switch", RELAY_PIN, SWITCH_PIN));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}