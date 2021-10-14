#pragma once

#include <sensors/analog.h>
#include "co2_sensor.h"

class IotCloud_CO2_T : public AnalogSensor
{

private:
    const float *_value;
    void get_value();

public:
    IotCloud_CO2_T(
        const char *sensor_id,
        const char *sensor_name,
        const char *metadata,
        const float *value);
};