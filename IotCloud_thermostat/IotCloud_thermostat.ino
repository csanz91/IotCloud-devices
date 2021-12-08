#include <Arduino.h>
#include <iotcloud.h>
#include "ds18b20_thermostat.h"

#define ONE_WIRE_BUS 5
#define RELAY_PIN 32

Device my_device = Device("v1.1", "ESP32_DS18B20_THERMOSTAT");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new IOTCLOUD_THERMOSTAT_DS18B20("Thermostat", "Thermostat", ONE_WIRE_BUS, RELAY_PIN));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}