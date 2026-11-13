#include "chain_waxer_sensor.h"
#include <cmath>

namespace
{
// Raw JSON: a lone \xB0 byte is not valid UTF-8 and the backend stores it as
// U+FFFD. The escape \u00b0 is ASCII and decodes to the degree sign.
constexpr char METADATA_JSON[] =
    "{\"engUnits\":\"\\u00b0C\",\"stringFormat\":\"#.0\"}";
}

ChainWaxerSensor::ChainWaxerSensor(
    const char *sensor_id,
    const char *sensor_name,
    uint8_t one_wire_bus,
    unsigned int report_period_ms)
    : AnalogSensor(
          sensor_id,
          sensor_name,
          report_period_ms,
          1,     // 1 decimal place
          false, // disable running avg rate-limiter so rapid heating isn't rejected
          "analog",
          METADATA_JSON),
      _one_wire_pin(one_wire_bus),
      _one_wire(one_wire_bus),
      _sensors(&_one_wire)
{
    pinMode(one_wire_bus, INPUT_PULLUP);
    delay(5);
    _sensors.begin();
    _sensors.setWaitForConversion(false);
}

void ChainWaxerSensor::loop_fast()
{
    const uint32_t now = millis();

    switch (_step)
    {
    case Step::FIND_SENSOR:
        if (now - _last_step_time >= RETRY_INTERVAL_MS)
        {
            _last_step_time = now;
            if (_sensors.getAddress(_sensor_address, 0))
            {
                _sensors.setResolution(_sensor_address, 12);
                _sensor_found = true;
                _sensors.requestTemperaturesByAddress(_sensor_address);
                _step = Step::WAIT_CONVERSION;
                _last_step_time = now;
            }
            else
            {
                _sensor_found = false;
                _current_temperature = NAN;
            }
        }
        break;

    case Step::REQUEST_CONVERSION:
        if (_sensor_found)
        {
            _sensors.requestTemperaturesByAddress(_sensor_address);
            _last_step_time = now;
            _step = Step::WAIT_CONVERSION;
        }
        else
        {
            _step = Step::FIND_SENSOR;
        }
        break;

    case Step::WAIT_CONVERSION:
        if (now - _last_step_time >= CONVERSION_DELAY_MS)
        {
            const float raw_temp = _sensors.getTempC(_sensor_address);
            if (raw_temp == DEVICE_DISCONNECTED_C ||
                std::isnan(raw_temp) ||
                std::isinf(raw_temp) ||
                raw_temp < -50.0f ||
                raw_temp > 135.0f)
            {
                _sensor_found = false;
                _raw_temperature = NAN;
                _current_temperature = NAN;
                _step = Step::FIND_SENSOR;
                _last_step_time = now;
            }
            else
            {
                _raw_temperature = raw_temp;
                _current_temperature = raw_temp + offset;

                if (_current_temperature < _min_temperature)
                {
                    _min_temperature = _current_temperature;
                }
                if (_current_temperature > _max_temperature)
                {
                    _max_temperature = _current_temperature;
                }

                _step = Step::REQUEST_CONVERSION;
            }
        }
        break;
    }

    update_soak_timer();
}

void ChainWaxerSensor::get_value()
{
    // AnalogSensor::set_value() adds the calibration offset itself, so pass
    // the uncorrected reading.
    if (_sensor_found && !std::isnan(_raw_temperature))
    {
        set_value(_raw_temperature);
    }
}

void ChainWaxerSensor::set_stage(WaxStage stage)
{
    if (_stage != stage)
    {
        _stage = stage;
        _timer_paused = false;
        reset_soak_timer();
    }
}

const char *ChainWaxerSensor::get_stage_name() const
{
    switch (_stage)
    {
    case WaxStage::STRIP_GREASE:
        return "Strip Grease (125°C)";
    case WaxStage::REGULAR_WAX:
        return "Regular Waxing (85°C)";
    case WaxStage::CHAIN_REMOVAL:
        return "Chain Removal (72°C)";
    case WaxStage::CUSTOM:
        return "Custom Target";
    case WaxStage::STANDBY:
    default:
        return "Standby / Monitor";
    }
}

float ChainWaxerSensor::get_target_temperature() const
{
    switch (_stage)
    {
    case WaxStage::STRIP_GREASE:
        return TEMP_STRIP_GREASE;
    case WaxStage::REGULAR_WAX:
        return TEMP_REGULAR_WAX;
    case WaxStage::CHAIN_REMOVAL:
        return TEMP_CHAIN_REMOVAL;
    case WaxStage::CUSTOM:
        return _custom_target;
    case WaxStage::STANDBY:
    default:
        return 0.0f;
    }
}

void ChainWaxerSensor::set_custom_target(float target)
{
    if (target >= 40.0f && target <= 130.0f)
    {
        _custom_target = target;
        set_stage(WaxStage::CUSTOM);
    }
}

void ChainWaxerSensor::set_calibration_offset(float new_offset)
{
    offset = new_offset;
}

const char *ChainWaxerSensor::get_pin_name() const
{
#if defined(D1)
    if (_one_wire_pin == D1) return "D1 (GPIO5)";
    if (_one_wire_pin == D2) return "D2 (GPIO4)";
    if (_one_wire_pin == D5) return "D5 (GPIO14)";
    if (_one_wire_pin == D6) return "D6 (GPIO12)";
#endif
    return "Custom Pin";
}

String ChainWaxerSensor::get_sensor_address_hex() const
{
    if (!_sensor_found)
    {
        return F("Not Detected");
    }

    char buf[17];
    for (uint8_t i = 0; i < 8; ++i)
    {
        snprintf(buf + (i * 2), 3, "%02X", _sensor_address[i]);
    }
    buf[16] = '\0';
    return String(buf);
}

void ChainWaxerSensor::reset_soak_timer()
{
    _soak_seconds = 0;
    _last_timer_tick = millis();
}

void ChainWaxerSensor::update_soak_timer()
{
    const uint32_t now = millis();

    if (_stage == WaxStage::STANDBY || !_sensor_found || std::isnan(_current_temperature))
    {
        _last_timer_tick = now;
        return;
    }

    // The timer counts only while the temperature is in the target zone and
    // the user hasn't paused it.
    if (is_in_target_zone() && !_timer_paused)
    {
        if (now - _last_timer_tick >= 1000)
        {
            const uint32_t elapsed_seconds = (now - _last_timer_tick) / 1000;
            _soak_seconds += elapsed_seconds;
            _last_timer_tick += elapsed_seconds * 1000;
        }
    }
    else
    {
        _last_timer_tick = now;
    }
}

bool ChainWaxerSensor::is_in_target_zone() const
{
    if (!_sensor_found || std::isnan(_current_temperature) || _stage == WaxStage::STANDBY)
    {
        return false;
    }

    const float target = get_target_temperature();

    switch (_stage)
    {
    case WaxStage::STRIP_GREASE:
        // High-temperature grease strip / oleogelation cycle around 125°C (120°C - 130°C)
        return _current_temperature >= 120.0f && _current_temperature <= 130.0f;

    case WaxStage::REGULAR_WAX:
        // Hot melt wax application range: 78°C - 90°C (target 85°C)
        return _current_temperature >= 78.0f && _current_temperature <= 90.0f;

    case WaxStage::CHAIN_REMOVAL:
        // Chain hang/pull range: 68°C - 75°C (target 72°C)
        return _current_temperature >= 68.0f && _current_temperature <= 75.0f;

    case WaxStage::CUSTOM:
        return fabsf(_current_temperature - target) <= 2.5f;

    case WaxStage::STANDBY:
    default:
        return false;
    }
}

float ChainWaxerSensor::get_temp_difference() const
{
    if (!_sensor_found || std::isnan(_current_temperature) || _stage == WaxStage::STANDBY)
    {
        return 0.0f;
    }
    return _current_temperature - get_target_temperature();
}

const char *ChainWaxerSensor::get_status_code() const
{
    if (!_sensor_found || std::isnan(_current_temperature))
    {
        return "sensor_error";
    }
    if (_stage == WaxStage::STANDBY)
    {
        return "standby";
    }

    if (is_in_target_zone())
    {
        if (_stage == WaxStage::CHAIN_REMOVAL)
        {
            return "ready_to_remove";
        }
        return "in_zone";
    }

    const float diff = get_temp_difference();
    if (_stage == WaxStage::CHAIN_REMOVAL)
    {
        if (diff > 0)
        {
            return "cooling_down";
        }
        return "too_cold";
    }

    if (diff < 0)
    {
        return "heating_up";
    }
    return "too_hot";
}

const char *ChainWaxerSensor::get_guidance_text() const
{
    if (!_sensor_found || std::isnan(_current_temperature))
    {
        return "DS18B20 sensor not detected. Check 1-Wire wiring & 4.7k pull-up resistor.";
    }

    switch (_stage)
    {
    case WaxStage::STRIP_GREASE:
        if (_current_temperature < 120.0f)
        {
            return "Heating pot... Target 125°C for factory grease breakdown.";
        }
        else if (_current_temperature > 130.0f)
        {
            return "Caution: Temperature above 130°C! Reduce heat immediately.";
        }
        else
        {
            return "Target 125°C reached! Submerge chain, agitate, and soak 10 min to dissolve grease.";
        }

    case WaxStage::REGULAR_WAX:
        if (_current_temperature < 78.0f)
        {
            return "Melting wax... Wait for 85°C before dropping in the chain.";
        }
        else if (_current_temperature > 92.0f)
        {
            return "Temperature high (>92°C). Reduce heat to avoid wax breakdown.";
        }
        else
        {
            return "Optimal wax soak (85°C)! Submerge chain, swish thoroughly, and soak for 5-10 min.";
        }

    case WaxStage::CHAIN_REMOVAL:
        if (_current_temperature > 75.0f)
        {
            return "Cooling down... Wait until 72°C before hanging chain for optimal wax coating.";
        }
        else if (_current_temperature < 68.0f)
        {
            return "Wax cooling below 68°C. Warm pot slightly if chain is not yet removed.";
        }
        else
        {
            return "Ready to pull! Slowly remove chain at 72°C and hang to cool.";
        }

    case WaxStage::CUSTOM:
        if (is_in_target_zone())
        {
            return "Target temperature reached! Maintaining custom setpoint.";
        }
        else if (_current_temperature < get_target_temperature())
        {
            return "Heating to custom target...";
        }
        else
        {
            return "Cooling to custom target...";
        }

    case WaxStage::STANDBY:
    default:
        return "Monitoring temperature. Select a waxing stage above to begin.";
    }
}
