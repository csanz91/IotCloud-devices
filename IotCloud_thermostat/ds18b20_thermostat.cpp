#include "ds18b20_thermostat.h"

IOTCLOUD_THERMOSTAT_DS18B20::IOTCLOUD_THERMOSTAT_DS18B20(
    const char *sensor_id,
    const char *sensor_name,
    const int one_wire_bus) : Thermostat(sensor_id,
                                         sensor_name,
                                         15000,
                                         1,
                                         true,
                                         "thermostat",
                                         "{\"engUnits\": \"\xB0"
                                         "C\", \"stringFormat\": \"#.0\"}")
{

    _oneWire = new OneWire(one_wire_bus);

    _sensors = new DallasTemperature(_oneWire);

    _sensors->begin();
    _sensors->setWaitForConversion(false);
}

void IOTCLOUD_THERMOSTAT_DS18B20::set_heating(bool new_heating_state)
{
    Thermostat::set_heating(new_heating_state);

    Serial.print("Heating: ");
    Serial.println(new_heating_state);
}

void IOTCLOUD_THERMOSTAT_DS18B20::get_value()
{
    switch (_current_step)
    {
    case Steps::GET_ADDR:
    {
        Serial.println(F("Searching devices..."));
        bool found = _sensors->getAddress(_sensor_addr, 0);
        if (found)
        {
            Serial.print(F("Device found"));
            _sensors->setResolution(_sensor_addr, 12);
            _current_step = Steps::REQ_TEMP;
        }
        else
        {
            break;
        }
    }
    case Steps::REQ_TEMP:
    {
        _sensors->requestTemperaturesByAddress(_sensor_addr);
        _current_step = Steps::GET_TEMP;
    }
    break;
    case Steps::GET_TEMP:
    {
        float temperature = _sensors->getTempC(_sensor_addr);
        if (temperature == DEVICE_DISCONNECTED_C)
        {
            Serial.println(F("Error: Could not read temperature data"));
            return;
        }
        Serial.println(temperature);
        set_value(temperature);
        _current_step = Steps::REQ_TEMP;
    }
    break;
    }
}
