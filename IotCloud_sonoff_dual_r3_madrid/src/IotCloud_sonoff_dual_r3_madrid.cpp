#include <Arduino.h>
#include <iotcloud.h>
#include "generic_switch.h"
#include "presence_sensor.h"

Device my_device = Device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);

static constexpr int PRESENCE_PIN = 17;

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new GENERIC_TOOGLE("Switch1", "Switch1", RELAY_1, SWITCH_1));
    my_device.add_sensor(new GENERIC_TOOGLE("Switch2", "Switch2", RELAY_2, SWITCH_2));
    my_device.add_sensor(new PRESENCE_SENSOR("Presence", "Presence", PRESENCE_PIN));
    iotcloud_setup(&my_device);
}

void loop(void)
{
    iotcloud_loop();
}
