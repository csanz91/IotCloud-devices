#include "light_sensor.h"

LIGHT_SENSOR::LIGHT_SENSOR(
    const char *sensor_id,
    const char *sensor_name,
    const char *metadata) : AnalogSensor(sensor_id,
                                       sensor_name,
                                       5000,
                                       2,
                                       false,
                                       "analog",
                                       metadata,
                                       0.00) // no filtering
{
    // Required to reset the sensor after an OTA update
    digitalWrite(D3, LOW);
    digitalWrite(D4, LOW);

    Wire.begin(D3, D4); // sda (blue), scl (green)
    lightMeter.begin();
}

void LIGHT_SENSOR::get_value()
{
    float read_value = lightMeter.readLightLevel();
    if (read_value >= 0.0 && !isnan(read_value))
        set_value(read_value);
}
