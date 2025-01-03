#pragma once

#include <sensors/analog.h>
#include <BH1750.h>

class LIGHT_SENSOR : public AnalogSensor
{

private:
    BH1750 lightMeter;
    float _value;
    void get_value();

public:
    LIGHT_SENSOR(
        const char *sensor_id,
        const char *sensor_name,
        const char *metadata);
};