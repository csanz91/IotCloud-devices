#include "sonoff_dual_r2_blinds.h"

#include <errno.h>
#include <math.h>

#include <device_data.h>

namespace
{
const char SENSOR_METADATA[] =
    "{\"bidirectional\":true,\"settings\":[{\"name\":\"Tiempo de apertura\",\"description\":\"Tiempo en milisegundos usado como base para interpolar movimientos parciales de subida.\",\"endpoint\":\"fullOpenTime\",\"actionType\":\"intpar\",\"value\":16667,\"limits\":{\"hi\":60000,\"lo\":1000}},{\"name\":\"Tiempo de cierre\",\"description\":\"Tiempo en milisegundos usado como base para interpolar movimientos de bajada.\",\"endpoint\":\"fullCloseTime\",\"actionType\":\"intpar\",\"value\":18333,\"limits\":{\"hi\":60000,\"lo\":1000}},{\"name\":\"Tiempo de apertura completa\",\"description\":\"Tiempo fijo en milisegundos usado solo cuando se solicita abrir al 100%.\",\"endpoint\":\"fullOpenCommandTime\",\"actionType\":\"intpar\",\"value\":23500,\"limits\":{\"hi\":60000,\"lo\":1000}},{\"name\":\"Desactivar LED de estado\",\"description\":\"Mantiene apagado el LED azul de estado.\",\"endpoint\":\"disableStatusLed\",\"actionType\":\"boolpar\",\"value\":false}]}";
constexpr long MIN_TIMING_MS = 1000;
constexpr long MAX_TIMING_MS = 60000;
}

SonoffDualR2Blinds::SonoffDualR2Blinds(
    const char *sensor_id,
    const char *sensor_name,
    int relay_up_pin,
    int relay_down_pin,
    int status_led_pin) : Toggle(sensor_id, sensor_name, SENSOR_METADATA, "blinds"),
                           _relay_up_pin(relay_up_pin),
                           _relay_down_pin(relay_down_pin),
                           _status_led_pin(status_led_pin)
{
    pinMode(_relay_up_pin, OUTPUT);
    pinMode(_relay_down_pin, OUTPUT);
    digitalWrite(_relay_up_pin, LOW);
    digitalWrite(_relay_down_pin, LOW);

    pinMode(_status_led_pin, OUTPUT);
    set_status_led(false);
}

void SonoffDualR2Blinds::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    BaseSensor::init(mqtt_header, mqtt_client);

    subscribe_command(mqtt_client, "aux/setMovement");

    subscribe_timing(mqtt_client, "aux/fullOpenTime", &_full_open_time);
    subscribe_timing(mqtt_client, "aux/fullCloseTime", &_full_close_time);
    subscribe_timing(mqtt_client, "aux/fullOpenCommandTime", &_full_open_command_time);
    subscribe_status_led_setting(mqtt_client);
    publish_movement();
}

void SonoffDualR2Blinds::loop()
{
    if (_direction != STOPPED && millis() - _movement_started_at >= _movement_duration)
    {
        stop_movement();
    }

    if (_direction == STOPPED && _last_direction != STOPPED &&
        millis() - _stopped_at >= REVERSAL_DEAD_TIME_MS)
    {
        // Dead time over. Clearing _last_direction also keeps a stale stop
        // time from looking recent again after millis() rolls over.
        _last_direction = STOPPED;
        if (_pending_direction != STOPPED)
        {
            Direction direction = _pending_direction;
            _pending_direction = STOPPED;
            energize(direction, _pending_duration);
        }
    }

    apply_relays();
}

void SonoffDualR2Blinds::set_toggle_state(ToggleState toggle_state)
{
    if (toggle_state == ToggleState::UP)
    {
        start_movement(OPENING, _full_open_command_time);
    }
    else if (toggle_state == ToggleState::DOWN)
    {
        start_movement(CLOSING, _full_close_time);
    }
    else if (toggle_state == ToggleState::TOGGLE)
    {
        return;
    }
}

void SonoffDualR2Blinds::subscribe_command(EspMQTTClient *mqtt_client, const char *endpoint)
{
    char constructed_topic[128] = "";
    construct_topic(constructed_topic, sizeof(constructed_topic), endpoint);
    mqtt_client->subscribe(constructed_topic, [&](const String &payload)
                           { handle_command(payload); });
}

void SonoffDualR2Blinds::subscribe_timing(EspMQTTClient *mqtt_client, const char *endpoint, unsigned long *target)
{
    char constructed_topic[128] = "";
    construct_topic(constructed_topic, sizeof(constructed_topic), endpoint);
    mqtt_client->subscribe(constructed_topic, [target](const String &payload)
                           {
                               String text = payload;
                               text.trim();
                               if (text.length() == 0)
                               {
                                   return;
                               }

                               errno = 0;
                               char *end = nullptr;
                               long value = strtol(text.c_str(), &end, 10);
                               if (errno == ERANGE || end == text.c_str() || *end != '\0' ||
                                   value < MIN_TIMING_MS || value > MAX_TIMING_MS)
                               {
                                   return;
                               }

                               *target = static_cast<unsigned long>(value);
                           });
}

void SonoffDualR2Blinds::subscribe_status_led_setting(EspMQTTClient *mqtt_client)
{
    char constructed_topic[128] = "";
    construct_topic(constructed_topic, sizeof(constructed_topic), "aux/disableStatusLed");
    mqtt_client->subscribe(constructed_topic, [&](const String &payload)
                           {
                               _status_led_enabled = !payload.equalsIgnoreCase("true");
                               if (!_status_led_enabled)
                               {
                                   digitalWrite(_status_led_pin, HIGH);
                               }
                           });
}

void SonoffDualR2Blinds::handle_command(const String &payload)
{
    float movement = 0.0f;

    if (!parse_movement(payload, &movement))
    {
        return;
    }

    if (movement == 0.0f)
    {
        stop_movement();
        return;
    }

    if (_direction != STOPPED || _pending_direction != STOPPED)
    {
        stop_movement();
        return;
    }

    float percent = abs(movement);
    if (movement > 0.0f)
    {
        unsigned long duration = movement_duration(OPENING, percent);
        if (duration >= MIN_MOVEMENT_DURATION_MS)
        {
            start_movement(OPENING, duration);
        }
    }
    else
    {
        unsigned long duration = movement_duration(CLOSING, percent);
        if (duration >= MIN_MOVEMENT_DURATION_MS)
        {
            start_movement(CLOSING, duration);
        }
    }
}

bool SonoffDualR2Blinds::parse_movement(const String &payload, float *movement)
{
    String value = payload;
    value.trim();

    if (value.length() == 0)
    {
        return false;
    }

    char *end = nullptr;
    float parsed = strtof(value.c_str(), &end);
    if (end == value.c_str() || *end != '\0' || !isfinite(parsed))
    {
        return false;
    }

    if (parsed > 100.0f)
    {
        parsed = 100.0f;
    }
    else if (parsed < -100.0f)
    {
        parsed = -100.0f;
    }

    *movement = parsed;
    return true;
}

unsigned long SonoffDualR2Blinds::movement_duration(Direction direction, float percent) const
{
    if (percent >= 100.0f)
    {
        return direction == OPENING ? _full_open_command_time : _full_close_time;
    }

    unsigned long full_time = direction == OPENING ? _full_open_time : _full_close_time;
    return static_cast<unsigned long>(full_time * percent / 100.0f);
}

void SonoffDualR2Blinds::start_movement(Direction direction, unsigned long duration)
{
    stop_movement();

    if (_last_direction != STOPPED && _last_direction != direction &&
        millis() - _stopped_at < REVERSAL_DEAD_TIME_MS)
    {
        // Reversing too soon: loop() starts it once the dead time has passed.
        _pending_direction = direction;
        _pending_duration = duration;
        return;
    }

    energize(direction, duration);
}

void SonoffDualR2Blinds::energize(Direction direction, unsigned long duration)
{
    _direction = direction;
    _movement_duration = duration;
    _movement_started_at = millis();
    apply_relays();
    publish_movement();
}

void SonoffDualR2Blinds::stop_movement()
{
    bool was_moving = _direction != STOPPED;
    _pending_direction = STOPPED;
    if (was_moving)
    {
        _last_direction = _direction;
        _stopped_at = millis();
    }
    _direction = STOPPED;
    _movement_duration = 0;
    digitalWrite(_relay_up_pin, LOW);
    digitalWrite(_relay_down_pin, LOW);
    set_status_led(false);

    if (was_moving)
    {
        publish_movement();
    }
}

void SonoffDualR2Blinds::publish_movement()
{
    if (_mqtt_client == nullptr || !_mqtt_client->isConnected())
    {
        return;
    }

    const char *movement = "stopped";
    if (_direction == OPENING)
    {
        movement = "opening";
    }
    else if (_direction == CLOSING)
    {
        movement = "closing";
    }

    char constructed_topic[128] = "";
    construct_topic(constructed_topic, sizeof(constructed_topic), "aux/movement");
    _mqtt_client->publish(constructed_topic, movement, true);
}

void SonoffDualR2Blinds::apply_relays()
{
    if (_direction == OPENING)
    {
        digitalWrite(_relay_down_pin, LOW);
        digitalWrite(_relay_up_pin, HIGH);
        set_status_led(true);
    }
    else if (_direction == CLOSING)
    {
        digitalWrite(_relay_up_pin, LOW);
        digitalWrite(_relay_down_pin, HIGH);
        set_status_led(true);
    }
    else
    {
        digitalWrite(_relay_up_pin, LOW);
        digitalWrite(_relay_down_pin, LOW);
        set_status_led(false);
    }
}

void SonoffDualR2Blinds::set_status_led(bool moving)
{
    if (!_status_led_enabled)
    {
        digitalWrite(_status_led_pin, HIGH);
        return;
    }

    digitalWrite(_status_led_pin, moving ? LOW : HIGH);
}
