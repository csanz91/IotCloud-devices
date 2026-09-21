#include "motion_light_sensor.h"

#include <Arduino.h>
#include <cstdlib>

#include "motion_feedback.h"

namespace
{
const char SENSOR_METADATA[] =
    "{\"settings\":[{\"name\":\"Motion timeout\",\"description\":\"Seconds to keep the light on after the latest motion webhook.\",\"endpoint\":\"motionTimeoutSeconds\",\"actionType\":\"intpar\",\"value\":30,\"limits\":{\"hi\":1800,\"lo\":10}}]}";
}

MotionLightSensor::MotionLightSensor(
    const char *sensor_id,
    const char *sensor_name,
    const uint8_t relay_pin,
    const uint8_t wall_switch_pin,
    const bool relay_active_high)
    : SwitchSensor(sensor_id, sensor_name, "switch", SENSOR_METADATA),
      _relay_pin(relay_pin),
      _wall_switch_pin(wall_switch_pin),
      _relay_active_high(relay_active_high)
{
    // Set the output latch to OFF before enabling the pin to avoid a relay pulse.
    digitalWrite(_relay_pin, _relay_active_high ? LOW : HIGH);
    pinMode(_relay_pin, OUTPUT);
    pinMode(_wall_switch_pin, INPUT_PULLUP);
}

void MotionLightSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    SwitchSensor::init(mqtt_header, mqtt_client);
    subscribe_timeout_setting(mqtt_client);
}

void MotionLightSensor::loop()
{
    const uint32_t now_ms = millis();
    update_wall_switch(now_ms);

    if (_controller.tick(now_ms))
    {
        apply_output();
    }
}

void MotionLightSensor::trigger_motion(const uint32_t now_ms)
{
    trigger_motion_feedback();

    if (_controller.trigger_motion(now_ms))
    {
        apply_output();
    }
}

void MotionLightSensor::set_state(const bool new_state)
{
    _controller.manual_set(new_state);
    apply_output(true);
}

void MotionLightSensor::apply_output(const bool force_report)
{
    const bool output_on = _controller.is_on();
    digitalWrite(
        _relay_pin,
        output_on == _relay_active_high ? HIGH : LOW);

    if (force_report || state != output_on)
    {
        SwitchSensor::set_state(output_on);
    }
}

void MotionLightSensor::update_wall_switch(const uint32_t now_ms)
{
    const bool reading = digitalRead(_wall_switch_pin);

    if (!_wall_initialized)
    {
        _wall_raw_state = reading;
        _wall_stable_state = reading;
        _wall_changed_at_ms = now_ms;
        _wall_initialized = true;
        return;
    }

    if (reading != _wall_raw_state)
    {
        _wall_raw_state = reading;
        _wall_changed_at_ms = now_ms;
    }

    if (reading == _wall_stable_state ||
        now_ms - _wall_changed_at_ms < WALL_SWITCH_DEBOUNCE_MS)
    {
        return;
    }

    _wall_stable_state = reading;
    _controller.manual_toggle();
    apply_output();
}

void MotionLightSensor::subscribe_timeout_setting(EspMQTTClient *mqtt_client)
{
    char topic[128] = "";
    construct_topic(topic, sizeof(topic), "aux/motionTimeoutSeconds");
    mqtt_client->subscribe(topic, [&](const String &payload) {
        String normalized = payload;
        normalized.trim();
        if (normalized.length() == 0)
        {
            return;
        }

        char *end = nullptr;
        const unsigned long requested = strtoul(normalized.c_str(), &end, 10);
        if (end == normalized.c_str() || *end != '\0' ||
            !_controller.set_timeout_seconds(requested))
        {
            Serial.println(F("Ignoring invalid motion timeout"));
            return;
        }

        Serial.print(F("Motion timeout set to "));
        Serial.print(_controller.timeout_seconds());
        Serial.println(F(" seconds"));
    });
}

