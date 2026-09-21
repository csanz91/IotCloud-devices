#include <Arduino.h>
#include <iotcloud.h>
#include <presence_sensor.h>

#include "generic_switch.h"

namespace
{
constexpr uint8_t RELAY_PIN = 12;
constexpr uint8_t PRESENCE_PIN = 2;
constexpr uint8_t MANUAL_SWITCH_PIN = 14;

Device my_device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
} // namespace

void setup()
{
    Serial.begin(115200);

    my_device.add_sensor(
        new GENERIC_TOOGLE("KitchenLight", "Switch", RELAY_PIN, MANUAL_SWITCH_PIN));
    my_device.add_sensor(
        new PRESENCE_SENSOR("KitchenPresence", "Pres. Cocina", PRESENCE_PIN));
    iotcloud_setup(&my_device);
}

void loop()
{
    iotcloud_loop();
}
