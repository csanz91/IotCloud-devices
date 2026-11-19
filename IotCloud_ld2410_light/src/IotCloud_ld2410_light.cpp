#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <iotcloud.h>
#include <presence_sensor.h>

#include "light_sensor.h"

namespace
{
    constexpr uint8_t LD2410_PIN = D1;
    constexpr uint8_t BH1750_SDA_PIN = D3; // blue
    constexpr uint8_t BH1750_SCL_PIN = D4; // green
    constexpr unsigned long LUX_SAMPLE_INTERVAL = 5000;

    BH1750 lightMeter;
    // Unknown until the first valid BH1750 sample; LIGHT_SENSOR skips NaN.
    float lux = NAN;

    Device my_device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
} // namespace

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new LIGHT_SENSOR("003_L", "Light", "{\"engUnits\": \"lux\", \"stringFormat\": \"#0.0\"}", &lux));
    my_device.add_sensor(new PRESENCE_SENSOR("004_P", "Presencia", LD2410_PIN));
    iotcloud_setup(&my_device);

    // Required to reset the sensor after an OTA update
    digitalWrite(BH1750_SDA_PIN, LOW);
    digitalWrite(BH1750_SCL_PIN, LOW);

    Wire.begin(BH1750_SDA_PIN, BH1750_SCL_PIN);
    lightMeter.begin();
}

void loop(void)
{
    iotcloud_loop();

    static unsigned long last_lux_sample_time = 0;
    const unsigned long now = millis();

    if (now - last_lux_sample_time >= LUX_SAMPLE_INTERVAL)
    {
        const float reading = lightMeter.readLightLevel();
        // readLightLevel() returns a negative value on error.
        if (reading >= 0.0f)
        {
            lux = reading;
        }
        Serial.print("Light: ");
        Serial.println(reading);

        last_lux_sample_time = now;
    }
}
