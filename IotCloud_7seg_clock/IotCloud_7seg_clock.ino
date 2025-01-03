#include <Arduino.h>
#include <iotcloud.h>
#include "seven_seg_clock.h"

Device my_device = Device("v1.4", "7SEG_CLOCK");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new Seven_Seg_Clock("001", "Clock"));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}