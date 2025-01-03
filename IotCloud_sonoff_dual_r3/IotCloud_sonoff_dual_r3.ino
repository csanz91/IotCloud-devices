#include <Arduino.h>
#include <iotcloud.h>
#include "generic_toogle.h"

Device my_device = Device("v1.0.4", "SONOFF_DUAL_R3");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new GENERIC_TOOGLE("SonoffDualR3", "Persiana", RELAY_1, RELAY_2, SWITCH_1, SWITCH_2));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}