#include "ds18b20_thermostat.h"

#include <cmath>

namespace
{
constexpr uint32_t REPORT_PERIOD_MS = 15000;
constexpr uint8_t TEMPERATURE_DECIMALS = 1;
constexpr float FILTER_RATE_CHANGE = 0.01F;
constexpr char THERMOSTAT_METADATA[] =
    "{\"engUnits\":\"\xB0"
    "C\",\"stringFormat\":\"#.0\"}";
} // namespace

IotCloudThermostatDs18b20::IotCloudThermostatDs18b20(
    const char *sensor_id,
    const char *sensor_name,
    uint8_t one_wire_bus,
    uint8_t relay_pin)
    : Thermostat(
          sensor_id,
          sensor_name,
          REPORT_PERIOD_MS,
          TEMPERATURE_DECIMALS,
          true,
          "thermostat",
          THERMOSTAT_METADATA,
          FILTER_RATE_CHANGE),
      _relay_pin(relay_pin),
      _one_wire(one_wire_bus),
      _sensors(&_one_wire)
{
    pinMode(_relay_pin, OUTPUT);
    digitalWrite(_relay_pin, HIGH);

    _sensors.begin();
    _sensors.setWaitForConversion(false);
}

void IotCloudThermostatDs18b20::set_heating(bool new_heating_state)
{
    Thermostat::set_heating(new_heating_state);
    digitalWrite(_relay_pin, new_heating_state ? LOW : HIGH);
}

void IotCloudThermostatDs18b20::get_value()
{
    switch (_current_step)
    {
    case Step::FIND_SENSOR:
        Serial.println(F("Searching for DS18B20 sensor..."));
        if (!_sensors.getAddress(_sensor_address, 0))
        {
            Serial.println(F("No DS18B20 sensor found"));
            return;
        }

        Serial.println(F("DS18B20 sensor found"));
        _sensors.setResolution(_sensor_address, 12);
        _current_step = Step::REQUEST_TEMPERATURE;
        [[fallthrough]];

    case Step::REQUEST_TEMPERATURE:
        _sensors.requestTemperaturesByAddress(_sensor_address);
        _current_step = Step::READ_TEMPERATURE;
        return;

    case Step::READ_TEMPERATURE:
    {
        const float temperature = _sensors.getTempC(_sensor_address);
        if (temperature == DEVICE_DISCONNECTED_C ||
            std::isnan(temperature) ||
            std::isinf(temperature))
        {
            Serial.println(F("Could not read DS18B20 temperature"));
            _current_step = Step::FIND_SENSOR;
            return;
        }

        set_value(temperature);
        _current_step = Step::REQUEST_TEMPERATURE;
        return;
    }
    }
}
