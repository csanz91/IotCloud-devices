#include <Arduino.h>
#include <iotcloud.h>
#include "subsensor_thermostat.h"

#define RELAY_PIN D2

Device my_device = Device("v1.0", "SUBSENSOR_THERMOSTAT");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new IOTCLOUD_THERMOSTAT_SUBSENSOR("Thermostat", "Thermostat", RELAY_PIN));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}