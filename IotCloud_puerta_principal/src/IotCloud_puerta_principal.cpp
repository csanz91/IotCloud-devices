#include <Arduino.h>
#include <iotcloud.h>
#include <sensors/toggle.h>

namespace
{
constexpr uint8_t RELAY_PIN = D2;
constexpr uint32_t RELAY_PULSE_TIME_MS = 500;
constexpr uint32_t OFFLINE_RESTART_TIMEOUT_MS = 15UL * 60UL * 1000UL;
constexpr char SENSOR_METADATA[] = "{\"bidirectional\":false}";

class MomentaryRelay : public Toggle
{
public:
    MomentaryRelay(
        const char *sensor_id,
        const char *sensor_name,
        uint8_t relay_pin,
        uint32_t pulse_time_ms)
        : Toggle(sensor_id, sensor_name, SENSOR_METADATA),
          _relay_pin(relay_pin),
          _pulse_time_ms(pulse_time_ms)
    {
        pinMode(_relay_pin, OUTPUT);
        digitalWrite(_relay_pin, LOW);
    }

    void loop() override
    {
        if (_active && millis() - _activated_at >= _pulse_time_ms)
        {
            digitalWrite(_relay_pin, LOW);
            _active = false;
        }
    }

protected:
    void set_toggle_state(ToggleState toggle_state) override
    {
        if (toggle_state == ToggleState::INACTIVE)
        {
            return;
        }

        digitalWrite(_relay_pin, HIGH);
        _activated_at = millis();
        _active = true;
    }

private:
    const uint8_t _relay_pin;
    const uint32_t _pulse_time_ms;
    uint32_t _activated_at = 0;
    bool _active = false;
};

Device device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
} // namespace

void setup()
{
    Serial.begin(115200);

    static MomentaryRelay puerta_principal(
        "puerta_principal",
        "Puerta",
        RELAY_PIN,
        RELAY_PULSE_TIME_MS);

    device.add_sensor(&puerta_principal);
    iotcloud_setup(&device, OFFLINE_RESTART_TIMEOUT_MS);
}

void loop()
{
    iotcloud_loop();
}
