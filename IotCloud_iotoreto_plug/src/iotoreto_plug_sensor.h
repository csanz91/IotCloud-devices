#pragma once

#include <sensors/sensor.h>

class IotoretoPlugSensor : public BaseSensor
{
private:
    static constexpr uint8_t FRAME_LEN = 24;
    static constexpr unsigned long DEBOUNCE_DELAY_MS = 50;
    static constexpr unsigned long REPORT_PERIOD_MS = 10000;
    static constexpr unsigned long HEALTH_REPORT_PERIOD_MS = 60000;
    static constexpr uint8_t MAX_SERIAL_BYTES_PER_LOOP = 64;

    const int _relay_pin;
    const int _switch_pin;
    const int _led_pin;

    uint8_t _rx_buf[FRAME_LEN];
    uint8_t _rx_count;

    bool _state;
    bool _status_led_enabled;
    bool _state_restored_from_rtc;
    bool _last_button_state;
    bool _button_was_pressed;
    unsigned long _last_debounce_time;
    unsigned long _last_report_ms;
    unsigned long _last_health_report_ms;

    float _voltage;
    float _current;
    float _power;
    float _energy_kwh;
    unsigned long _last_frame_ms;

    bool restore_state();
    void persist_state();
    bool parse_frame();
    void read_meter();
    void handle_button();
    void set_state(bool state);
    void set_status_led(bool on);
    void report_state();
    void report_power();
    void report_health();
    void report_aux_metric(const char *endpoint, float value, unsigned short decimals);
    void subscribe_topics(EspMQTTClient *mqtt_client);

public:
    IotoretoPlugSensor(
        const char *sensor_id,
        const char *sensor_name,
        const int relay_pin,
        const int switch_pin,
        const int led_pin = -1);

    void init(char *mqtt_header, EspMQTTClient *mqtt_client) override;
    void loop() override;
};
