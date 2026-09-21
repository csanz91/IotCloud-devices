#include <Arduino.h>
#include <iotcloud.h>
#include <presence_sensor.h>

#include "generic_switch.h"

namespace
{
constexpr uint8_t RELAY_PIN = D2;
constexpr uint8_t LD2410_PIN = D6;
constexpr uint8_t MANUAL_SWITCH_PIN = D1;

Device my_device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
} // namespace

void setup()
{
    Serial.begin(115200);

    my_device.add_sensor(
        new GENERIC_TOOGLE("Switch", "Luz Salon Central", RELAY_PIN, MANUAL_SWITCH_PIN));
    my_device.add_sensor(
        new PRESENCE_SENSOR("Presence", "Presencia Salon Central", LD2410_PIN));
    iotcloud_setup(&my_device);
}

void loop()
{
    iotcloud_loop();
}
