#pragma once

#include <DallasTemperature.h>
#include <OneWire.h>
#include <sensors/thermostat.h>

class IotCloudThermostatDs18b20 : public Thermostat
{
public:
    IotCloudThermostatDs18b20(
        const char *sensor_id,
        const char *sensor_name,
        uint8_t one_wire_bus,
        uint8_t relay_pin);

protected:
    void get_value() override;
    void set_heating(bool new_heating_state) override;

private:
    enum class Step : uint8_t
    {
        FIND_SENSOR,
        REQUEST_TEMPERATURE,
        READ_TEMPERATURE,
    };

    const uint8_t _relay_pin;
    OneWire _one_wire;
    DallasTemperature _sensors;
    DeviceAddress _sensor_address{};
    Step _current_step = Step::FIND_SENSOR;
};
