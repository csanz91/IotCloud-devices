#include <Arduino.h>
#include <iotcloud.h>

#include "rgb_lamp.h"

Device my_device = Device("v1.7", "RGB_RAINBOW");

#define NUM_LEDS 110

void setup(void)
{
    Serial.begin(115200);
    my_device.add_sensor(new RgbLamp("001", "Yeah!", NUM_LEDS));

    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}