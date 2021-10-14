#include <Arduino.h>
#include <iotcloud.h>

#include "hexa_led.h"

Device my_device = Device("v1.0", "HEXALED");

void setup(void)
{
    Serial.begin(115200);
    my_device.add_sensor(new HexaLed("001", "Hexaled"));

    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}