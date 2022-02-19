#pragma once

#include <sensors/analog.h>
#include <ds18b20.h>

class IOTCLOUD_DS18B20 : public AnalogSensor
{

private:
    DS18B20 *_ds18b20;

public:
    void get_value();
    IOTCLOUD_DS18B20(
        const char *sensor_id,
        const char *sensor_name,
        const int one_wire_bus);
};