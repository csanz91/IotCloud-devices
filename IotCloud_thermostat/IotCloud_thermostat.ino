#include <Arduino.h>
#include <iotcloud.h>
#include "ds18b20_thermostat.h"

#define ONE_WIRE_BUS D1

Device my_device = Device("v1.1", "DS18B20_THERMOSTAT");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new IOTCLOUD_THERMOSTAT_DS18B20("001_T", "Thermostat", ONE_WIRE_BUS));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}