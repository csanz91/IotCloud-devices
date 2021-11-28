#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>

#include <sensors/thermostat.h>

class IOTCLOUD_THERMOSTAT_DS18B20 : public Thermostat
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
    void set_heating(bool);

public:
    IOTCLOUD_THERMOSTAT_DS18B20(
        const char *sensor_id,
        const char *sensor_name,
        const int one_wire_bus);
};