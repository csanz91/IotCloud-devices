#pragma once

#include <Arduino.h>
#include <sensors/toggle.h>

class SonoffDualR2Blinds : public Toggle
{
public:
    SonoffDualR2Blinds(
        const char *sensor_id,
        const char *sensor_name,
        int relay_up_pin,
        int relay_down_pin,
        int status_led_pin);

    void init(char *mqtt_header, EspMQTTClient *mqtt_client) override;
    void loop() override;

protected:
    void set_toggle_state(ToggleState toggle_state) override;

private:
    enum Direction
    {
        STOPPED,
        OPENING,
        CLOSING
    };

    static constexpr unsigned long MIN_MOVEMENT_DURATION_MS = 250;

    const int _relay_up_pin;
    const int _relay_down_pin;
    const int _status_led_pin;

    unsigned long _full_open_time = 16667;
    unsigned long _full_close_time = 18333;
    unsigned long _full_open_command_time = 23500;
    Direction _direction = STOPPED;
    unsigned long _movement_started_at = 0;
    unsigned long _movement_duration = 0;
    bool _status_led_enabled = true;

    void subscribe_command(EspMQTTClient *mqtt_client, const char *endpoint);
    void subscribe_timing(EspMQTTClient *mqtt_client, const char *endpoint, unsigned long *target);
    void subscribe_status_led_setting(EspMQTTClient *mqtt_client);
    void handle_command(const String &payload);
    bool parse_movement(const String &payload, float *movement);
    unsigned long movement_duration(Direction direction, float percent) const;
    void start_movement(Direction direction, unsigned long duration);
    void stop_movement();
    void publish_movement();
    void apply_relays();
    void set_status_led(bool moving);
};
