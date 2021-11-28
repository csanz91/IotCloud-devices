#include "subsensor_thermostat.h"

IOTCLOUD_THERMOSTAT_SUBSENSOR::IOTCLOUD_THERMOSTAT_SUBSENSOR(
    const char *sensor_id,
    const char *sensor_name,
    const int relay_pin) : Thermostat(sensor_id,
                                      sensor_name,
                                      15000,
                                      1,
                                      true,
                                      "thermostat",
                                      "{\"engUnits\": \"\xB0"
                                      "C\", \"stringFormat\": \"#.0\"}"),
                           _relay_pin(relay_pin)
{
    pinMode(_relay_pin, OUTPUT);
}

void IOTCLOUD_THERMOSTAT_SUBSENSOR::set_heating(bool new_heating_state)
{
    Thermostat::set_heating(new_heating_state);
    digitalWrite(_relay_pin, new_heating_state);
}