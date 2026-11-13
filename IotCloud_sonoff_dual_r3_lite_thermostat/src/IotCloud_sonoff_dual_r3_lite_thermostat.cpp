#include <Arduino.h>
#include <iotcloud.h>

#include "ds18b20_thermostat.h"

namespace
{
// DS18B20 data pin: the same free pad the Madrid presence sensor uses.
// Wire 3V3, GND and DATA, with a 4.7 k pull-up from DATA to 3V3.
constexpr uint8_t ONE_WIRE_BUS = 17;
// Heating output. GPIO14 (RELAY_2) lit LED2 but never clicked, so this tries
// RELAY_1 (GPIO27); the other relay driver is held low in setup().
constexpr uint8_t HEATING_RELAY_PIN = RELAY_1;
constexpr uint8_t UNUSED_RELAY_PIN = RELAY_2;
constexpr uint32_t OFFLINE_RESTART_TIMEOUT_MS = 15UL * 60UL * 1000UL;

Device device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
} // namespace

void setup()
{
    Serial.begin(115200);

    // Keep the other relay's driver low. The S1/S2 inputs (GPIO32/33) are
    // broken and deliberately left unused.
    pinMode(UNUSED_RELAY_PIN, OUTPUT);
    digitalWrite(UNUSED_RELAY_PIN, LOW);

    static IotCloudThermostatDs18b20 thermostat(
        "Thermostat",
        "Thermostat",
        ONE_WIRE_BUS,
        HEATING_RELAY_PIN);

    device.add_sensor(&thermostat);
    iotcloud_setup(&device, OFFLINE_RESTART_TIMEOUT_MS);
}

void loop()
{
    iotcloud_loop();
}
