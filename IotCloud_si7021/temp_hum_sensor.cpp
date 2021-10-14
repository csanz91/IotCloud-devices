#include "temp_hum_sensor.h"

IotCloud_T_H::IotCloud_T_H(
    const char *sensor_id,
    const char *sensor_name,
    const char *metadata,
    const float *value) : AnalogSensor(sensor_id,
                                       sensor_name,
                                       30000,
                                       1,
                                       true,
                                       "analog",
                                       metadata,
                                       0.05),
                          _value(value)
{
}

void IotCloud_T_H::get_value()
{
    float read_value = *_value;
    if (!isnan(read_value))
    {
        set_value(read_value);
    }
}
