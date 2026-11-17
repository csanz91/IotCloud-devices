#pragma once

#include <Arduino.h>
#include <sensors/toggle.h>

#include "blinds_core.h"

// MQTT/GPIO adapter around BlindsController. All timing, debounce and safety
// logic lives in blinds_core.cpp (host-tested); this class only reads pins,
// writes relays and wires MQTT topics.
class SonoffDualR3Blinds : public Toggle
{
public:
    SonoffDualR3Blinds(
        const char *sensor_id,
        const char *sensor_name,
        int relay_up_pin,
        int relay_down_pin,
        int switch_up_pin,
        int switch_down_pin);

    void init(char *mqtt_header, EspMQTTClient *mqtt_client) override;
    void loop() override;

protected:
    void set_toggle_state(ToggleState toggle_state) override;

private:
    const int _relay_up_pin;
    const int _relay_down_pin;
    const int _switch_up_pin;
    const int _switch_down_pin;

    BlindsController _blinds;
    bool _begun = false;
    BlindsDirection _published_direction = BlindsDirection::STOPPED;
    bool _published_lockout = false;
    bool _status_published = false;

    void handle_movement_command(const String &payload);
    void subscribe_timing(EspMQTTClient *mqtt_client, const char *endpoint, bool (BlindsController::*setter)(uint32_t));
    void apply_outputs();
    void publish_status(bool force);
};
