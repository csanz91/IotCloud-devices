#include "ds18b20_sensor.h"

IOTCLOUD_DS18B20::IOTCLOUD_DS18B20(
    const char *sensor_id,
    const char *sensor_name,
    const int one_wire_bus) : AnalogSensor(sensor_id,
                                           sensor_name,
                                           15000,
                                           1,
                                           true,
                                           "analog",
                                           "{\"engUnits\": \"\xB0"
                                           "C\", \"stringFormat\": \"0.0\"}")
{

    _ds18b20 = new DS18B20(one_wire_bus);
}

void IOTCLOUD_DS18B20::get_value()
{
    float value = _ds18b20->get_value();
    if (value != NULL)
    {
        set_value(value);
    }
}
