#pragma once

#include <sensors/analog.h>
#include "si7021_am2301.h"

class IotCloud_T_H : public AnalogSensor
{

private:
    const float *_value;
    void get_value();

public:
    IotCloud_T_H(
        const char *sensor_id,
        const char *sensor_name,
        const char *metadata,
        const float *value);
};