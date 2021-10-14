#include "co2_sensor.h"

IotCloud_CO2_T::IotCloud_CO2_T(
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
                                       0.0),
                          _value(value)
{
}

void IotCloud_CO2_T::get_value()
{
    float read_value = *_value;
    if (!isnan(read_value))
    {
        set_value(read_value);
    }
}
