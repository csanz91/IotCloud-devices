#pragma once

#include <Arduino.h>
#include <sensors/analog.h>
#include <OneWire.h>
#include <DallasTemperature.h>

enum class WaxStage : uint8_t
{
    STANDBY = 0,
    STRIP_GREASE = 1,  // Factory grease stripping (125°C)
    REGULAR_WAX = 2,   // Regular hot melt immersion (85°C)
    CHAIN_REMOVAL = 3, // Optimal hang/pull (72°C)
    CUSTOM = 4
};

class ChainWaxerSensor : public AnalogSensor
{
public:
    static constexpr float TEMP_STRIP_GREASE = 125.0f;
    static constexpr float TEMP_REGULAR_WAX = 85.0f;
    static constexpr float TEMP_CHAIN_REMOVAL = 72.0f;

    ChainWaxerSensor(
        const char *sensor_id,
        const char *sensor_name,
        uint8_t one_wire_bus,
        unsigned int report_period_ms = 5000);

    // Call from main loop() for fast, non-blocking DS18B20 sampling
    void loop_fast();

    // Called by AnalogSensor::loop() periodically to publish to IoTCloud MQTT
    void get_value() override;

    // Stage & Target management
    void set_stage(WaxStage stage);
    WaxStage get_stage() const { return _stage; }
    const char *get_stage_name() const;
    float get_target_temperature() const;
    void set_custom_target(float target);
    float get_custom_target() const { return _custom_target; }

    // Temperature & Diagnostics
    float get_current_temperature() const { return _current_temperature; }
    bool is_sensor_connected() const { return _sensor_found; }
    String get_sensor_address_hex() const;
    float get_min_temperature() const { return _min_temperature; }
    float get_max_temperature() const { return _max_temperature; }
    float get_calibration_offset() const { return offset; }
    void set_calibration_offset(float new_offset);
    const char *get_pin_name() const;

    // Soak / Immersion Timer
    uint32_t get_soak_seconds() const { return _soak_seconds; }
    bool is_timer_running() const { return _timer_running; }
    void set_timer_running(bool running) { _timer_running = running; }
    void reset_soak_timer();

    // Process State & User Guidance
    bool is_in_target_zone() const;
    float get_temp_difference() const; // current - target
    const char *get_status_code() const;
    const char *get_guidance_text() const;

private:
    enum class Step : uint8_t
    {
        FIND_SENSOR,
        REQUEST_CONVERSION,
        WAIT_CONVERSION
    };

    static constexpr uint32_t CONVERSION_DELAY_MS = 750; // 12-bit Dallas conversion
    static constexpr uint32_t RETRY_INTERVAL_MS = 2000;

    uint8_t _one_wire_pin = 0;
    OneWire _one_wire;
    DallasTemperature _sensors;
    DeviceAddress _sensor_address{};
    bool _sensor_found = false;
    Step _step = Step::FIND_SENSOR;
    uint32_t _last_step_time = 0;

    float _current_temperature = NAN;
    float _min_temperature = 999.0f;
    float _max_temperature = -999.0f;

    WaxStage _stage = WaxStage::REGULAR_WAX;
    float _custom_target = 85.0f;

    // Timer tracking
    uint32_t _soak_seconds = 0;
    uint32_t _last_timer_tick = 0;
    bool _timer_running = false;
    bool _target_reached_event = false;

    void update_soak_timer();
};
