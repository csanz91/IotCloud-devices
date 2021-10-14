#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>

#include <sensors/analog.h>

class IOTCLOUD_DS18B20 : public AnalogSensor
{
    enum Steps
    {
        GET_ADDR,
        REQ_TEMP,
        GET_TEMP
    };

private:
    DallasTemperature *_sensors;
    DeviceAddress _sensor_addr;
    OneWire *_oneWire;
    Steps _current_step = Steps::GET_ADDR;
    void get_value();

public:
    IOTCLOUD_DS18B20(
        const char *sensor_id,
        const char *sensor_name,
        const int one_wire_bus);
};