#include <Arduino.h>
#include <iotcloud.h>
#include "generic_switch.h"

#define RELAY_PIN 12
#define SWITCH_PIN 4

Device my_device = Device("v1.2.5", "SONOFF_MINI");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new GENERIC_TOOGLE("SonoffMini", "Switch", RELAY_PIN, SWITCH_PIN));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}