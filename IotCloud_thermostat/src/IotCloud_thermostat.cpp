#include <Arduino.h>
#include <iotcloud.h>

#include "ds18b20_thermostat.h"

namespace
{
constexpr uint8_t ONE_WIRE_BUS = 5;
constexpr uint8_t RELAY_PIN = 32;
constexpr uint32_t OFFLINE_RESTART_TIMEOUT_MS = 15UL * 60UL * 1000UL;

Device device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
} // namespace

void setup()
{
    Serial.begin(115200);

    static IotCloudThermostatDs18b20 thermostat(
        "Thermostat",
        "Thermostat",
        ONE_WIRE_BUS,
        RELAY_PIN);

    device.add_sensor(&thermostat);
    iotcloud_setup(&device, OFFLINE_RESTART_TIMEOUT_MS);
}

void loop()
{
    iotcloud_loop();
}
