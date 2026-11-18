#include <Arduino.h>
#include <iotcloud.h>
#include <sensors/thermostat.h>

namespace
{
constexpr uint8_t RELAY_PIN = D2; // GPIO4, active-high
constexpr uint32_t OFFLINE_RESTART_TIMEOUT_MS = 15UL * 60UL * 1000UL;
constexpr char THERMOSTAT_METADATA[] = R"({"engUnits":"\u00b0C","stringFormat":"#.0"})";

class MadridThermostat : public Thermostat
{
public:
    MadridThermostat()
        : Thermostat("Thermostat", "Thermostat", 15000, 1, true, "thermostat", THERMOSTAT_METADATA)
    {
        digitalWrite(RELAY_PIN, LOW);
        pinMode(RELAY_PIN, OUTPUT);
    }

    void init(char *mqtt_header, EspMQTTClient *mqtt_client) override
    {
        Thermostat::init(mqtt_header, mqtt_client);

        // Keep the sensor version topic published by the original relay firmware.
        char topic[128];
        construct_topic(topic, sizeof(topic), "aux/thermostat");
        mqtt_client->publish(topic, version, true);
    }

private:
    void set_heating(bool new_heating_state) override
    {
        digitalWrite(RELAY_PIN, new_heating_state ? HIGH : LOW);
        Thermostat::set_heating(new_heating_state);
    }
};

Device device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
} // namespace

void setup()
{
    Serial.begin(115200);

    static MadridThermostat thermostat;
    device.add_sensor(&thermostat);
    iotcloud_setup(&device, OFFLINE_RESTART_TIMEOUT_MS);
}

void loop()
{
    iotcloud_loop();
}
