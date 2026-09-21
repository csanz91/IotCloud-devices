#include <Arduino.h>
#include <iotcloud.h>
#include "ds18b20_sensor.h"

#define ONE_WIRE_BUS D1

Device my_device = Device("v1.0", "DS18B20");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new IOTCLOUD_DS18B20("001_T", "Temperature", ONE_WIRE_BUS));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}