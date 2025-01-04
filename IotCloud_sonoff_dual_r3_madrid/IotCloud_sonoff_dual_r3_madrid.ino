#include <Arduino.h>
#include <iotcloud.h>
#include "generic_switch.h"
#include "presence_sensor.h"

Device my_device = Device("v1.0.3", "SONOFF_DUAL_R3_MADRID");

// boards.txt -> sonoff_dualr3.build.variant=sonoff_dualr3
// static const uint8_t LED_BUILTIN = 13; -> esp32/3.0.7/variants/sonoff_dualr3/pins_arduino.h
// de lo contrario no se cargan las definiciones de los pines

#define TX2 17
#define RX2 16

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new GENERIC_TOOGLE("Switch1", "Switch1", RELAY_1, SWITCH_1));
    my_device.add_sensor(new GENERIC_TOOGLE("Switch2", "Switch2", RELAY_2, SWITCH_2));
    my_device.add_sensor(new PRESENCE_SENSOR("Presence", "Presence", TX2));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}