#include <Arduino.h>
#include <iotcloud.h>

#include "generic_switch.h"

namespace
{
constexpr uint8_t RELAY_PIN = 4;
constexpr uint8_t SWITCH_PIN = 5;

Device my_device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
} // namespace

void setup()
{
    Serial.begin(115200);

    my_device.add_sensor(
        new GENERIC_TOOGLE("Shelly1", "Switch", RELAY_PIN, SWITCH_PIN));
    iotcloud_setup(&my_device);
}

void loop()
{
    iotcloud_loop();
}
