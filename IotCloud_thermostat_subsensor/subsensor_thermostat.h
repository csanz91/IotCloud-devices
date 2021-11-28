#pragma once

#include <sensors/thermostat.h>

class IOTCLOUD_THERMOSTAT_SUBSENSOR : public Thermostat
{
private:
    const int _relay_pin;
    void set_heating(bool);

public:
    IOTCLOUD_THERMOSTAT_SUBSENSOR(
        const char *sensor_id,
        const char *sensor_name,
        const int relay_pin);
};