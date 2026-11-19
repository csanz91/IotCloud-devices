#include "light_sensor.h"

LIGHT_SENSOR::LIGHT_SENSOR(
    const char *sensor_id,
    const char *sensor_name,
    const char *metadata,
    const float *value) : AnalogSensor(sensor_id,
                                       sensor_name,
                                       5000,
                                       2,
                                       false,
                                       "analog",
                                       metadata,
                                       0.00), // no filtering
                          _value(value)
{
}

void LIGHT_SENSOR::get_value()
{
    float read_value = *_value;
    if (read_value >= 0.0 && !isnan(read_value))
        set_value(read_value);
}
