#pragma once

#include <sensors/analog.h>

class LIGHT_SENSOR : public AnalogSensor
{

private:
    const float *_value;
    void get_value();

public:
    LIGHT_SENSOR(
        const char *sensor_id,
        const char *sensor_name,
        const char *metadata,
        const float *value);
};