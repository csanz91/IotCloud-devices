#include <Arduino.h>
#include <iotcloud.h>

#include "co2_sensor.h"

// pin for uart reading
#define MH_Z19_RX D7 // D7
#define MH_Z19_TX D6 // D6

Device my_device = Device("v1.2", "CO2");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new IotCloud_CO2("001_CO2", "CO2", "{\"engUnits\": \"ppm\", \"stringFormat\": \"#0\"}", MH_Z19_RX, MH_Z19_TX));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}