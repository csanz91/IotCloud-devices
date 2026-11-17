#include "sonoff_dual_r3_blinds.h"

namespace
{
const char SENSOR_METADATA[] =
    "{\"bidirectional\":true,\"settings\":["
    "{\"name\":\"Tiempo de apertura\",\"description\":\"Tiempo en milisegundos usado como base para interpolar movimientos parciales de subida.\",\"endpoint\":\"fullOpenTime\",\"actionType\":\"intpar\",\"value\":20000,\"limits\":{\"hi\":60000,\"lo\":1000}},"
    "{\"name\":\"Tiempo de cierre\",\"description\":\"Tiempo en milisegundos usado como base para interpolar movimientos de bajada.\",\"endpoint\":\"fullCloseTime\",\"actionType\":\"intpar\",\"value\":18333,\"limits\":{\"hi\":60000,\"lo\":1000}},"
    "{\"name\":\"Tiempo de apertura completa\",\"description\":\"Tiempo fijo en milisegundos usado solo cuando se solicita abrir al 100%.\",\"endpoint\":\"fullOpenCommandTime\",\"actionType\":\"intpar\",\"value\":23500,\"limits\":{\"hi\":60000,\"lo\":1000}}"
    "]}";

// Wall switches pull the input to GND when active.
inline bool switch_active(int pin)
{
    return digitalRead(pin) == LOW;
}
} // namespace

SonoffDualR3Blinds::SonoffDualR3Blinds(
    const char *sensor_id,
    const char *sensor_name,
    int relay_up_pin,
    int relay_down_pin,
    int switch_up_pin,
    int switch_down_pin) : Toggle(sensor_id, sensor_name, SENSOR_METADATA, "blinds"),
                           _relay_up_pin(relay_up_pin),
                           _relay_down_pin(relay_down_pin),
                           _switch_up_pin(switch_up_pin),
                           _switch_down_pin(switch_down_pin)
{
    pinMode(_relay_up_pin, OUTPUT);
    pinMode(_relay_down_pin, OUTPUT);
    digitalWrite(_relay_up_pin, LOW);
    digitalWrite(_relay_down_pin, LOW);

    pinMode(_switch_up_pin, INPUT_PULLUP);
    pinMode(_switch_down_pin, INPUT_PULLUP);
}

void SonoffDualR3Blinds::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    // Toggle::init subscribes to aux/setToggle ("up" / "down" / "toggle") and
    // aux/actionTime (unused by this device).
    Toggle::init(mqtt_header, mqtt_client);

    char topic[128] = "";

    construct_topic(topic, sizeof(topic), "aux/setMovement");
    mqtt_client->subscribe(topic, [this](const String &payload)
                           { handle_movement_command(payload); });

    subscribe_timing(mqtt_client, "aux/fullOpenTime", &BlindsController::set_full_open_time);
    subscribe_timing(mqtt_client, "aux/fullCloseTime", &BlindsController::set_full_close_time);
    subscribe_timing(mqtt_client, "aux/fullOpenCommandTime", &BlindsController::set_full_open_command_time);

    // Retained status is re-published on every (re)connection.
    publish_status(true);
}

void SonoffDualR3Blinds::loop()
{
    const uint32_t now = millis();
    const bool up = switch_active(_switch_up_pin);
    const bool down = switch_active(_switch_down_pin);

    if (!_begun)
    {
        _blinds.begin(now, up, down);
        _begun = true;
    }

    _blinds.tick(now, up, down);
    apply_outputs();
}

void SonoffDualR3Blinds::set_toggle_state(ToggleState toggle_state)
{
    if (toggle_state == ToggleState::UP)
    {
        _blinds.command_movement(100.0f, millis());
    }
    else if (toggle_state == ToggleState::DOWN)
    {
        _blinds.command_movement(-100.0f, millis());
    }
    else
    {
        // "toggle" on a blind: treat it as stop.
        _blinds.command_stop(millis());
    }
    apply_outputs();
}

void SonoffDualR3Blinds::handle_movement_command(const String &payload)
{
    float movement = 0.0f;
    if (!BlindsController::parse_movement(payload.c_str(), &movement))
    {
        return;
    }
    _blinds.command_movement(movement, millis());
    apply_outputs();
}

void SonoffDualR3Blinds::subscribe_timing(
    EspMQTTClient *mqtt_client,
    const char *endpoint,
    bool (BlindsController::*setter)(uint32_t))
{
    char topic[128] = "";
    construct_topic(topic, sizeof(topic), endpoint);
    mqtt_client->subscribe(topic, [this, setter](const String &payload)
                           {
                               uint32_t value = 0;
                               if (BlindsController::parse_timing(payload.c_str(), &value))
                               {
                                   (_blinds.*setter)(value);
                               } });
}

void SonoffDualR3Blinds::apply_outputs()
{
    // Always release before energizing. The controller already guarantees
    // the two are never requested together and enforces the reversal dead
    // time, so this order only matters within a single call.
    const bool up = _blinds.relay_up();
    const bool down = _blinds.relay_down();
    if (!up)
    {
        digitalWrite(_relay_up_pin, LOW);
    }
    if (!down)
    {
        digitalWrite(_relay_down_pin, LOW);
    }
    if (up)
    {
        digitalWrite(_relay_up_pin, HIGH);
    }
    if (down)
    {
        digitalWrite(_relay_down_pin, HIGH);
    }

    publish_status(false);
}

void SonoffDualR3Blinds::publish_status(bool force)
{
    if (_mqtt_client == nullptr || !_mqtt_client->isConnected())
    {
        // Publish again once connected; init() forces it.
        return;
    }

    const BlindsDirection direction = _blinds.direction();
    const bool lockout = _blinds.switch_lockout();
    char topic[128] = "";

    if (force || !_status_published || direction != _published_direction)
    {
        const char *movement = "stopped";
        if (direction == BlindsDirection::OPENING)
        {
            movement = "opening";
        }
        else if (direction == BlindsDirection::CLOSING)
        {
            movement = "closing";
        }
        construct_topic(topic, sizeof(topic), "aux/movement");
        _mqtt_client->publish(topic, movement, true);
        _published_direction = direction;
    }

    if (force || !_status_published || lockout != _published_lockout)
    {
        topic[0] = '\0';
        construct_topic(topic, sizeof(topic), "aux/switchLockout");
        _mqtt_client->publish(topic, lockout ? "true" : "false", true);
        _published_lockout = lockout;
    }

    _status_published = true;
}
