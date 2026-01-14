#include "generic_switch.h"

GENERIC_TOOGLE::GENERIC_TOOGLE(
    const char *sensor_id,
    const char *sensor_name,
    const int relay_pin,
    const int switch_pin,
    NOTIFIER *notifier,
    float *lux) : SwitchSensor(sensor_id,
                                       sensor_name,
                                       "switch",
                                       "{}"), 
                          _relay_pin(relay_pin),
                          _switch_pin(switch_pin),
                          _notifier(notifier),
                          _lux(lux)
{

    pinMode(_relay_pin, OUTPUT);
    digitalWrite(_relay_pin, LOW);

    pinMode(_switch_pin, INPUT_PULLUP);

    _last_state = digitalRead(_switch_pin);

    _setup_time = millis();
}

void GENERIC_TOOGLE::set_state(const bool state)
{
    Serial.print("Setting state to: ");
    Serial.println(state);
    SwitchSensor::set_state(state);
    digitalWrite(_relay_pin, state);
}

void GENERIC_TOOGLE::loop()
{
    const unsigned long now = millis();

    // Ignore the inputs during the first seconds. Avoid false triggers
    if (now - _setup_time < START_DELAY)
    {
        return;
    }

    // Debounce the switch
    bool switch_state = digitalRead(_switch_pin);
    if (switch_state != _last_state)
    {
        _mem_switch_state_1 = true;
        _last_debounce_time_1 = now;
    }

    if ((now - _last_debounce_time_1) > 50)
    {
        if (_mem_switch_state_1)
        {
            _mem_switch_state_1 = false;

            report_presence(switch_state);

            if (!disable_notifications && (now - _last_notification_time > _notifications_cooldown))
            {
                _notifier->send_notification("Movimiento detectado en: {sensor_name}");
                _last_notification_time = now;
            }

            if (*_lux >= 0.0 && !isnan(*_lux) && *_lux < _luminosity_threshold)
            {
                GENERIC_TOOGLE::set_state(switch_state);
            }

        }
    }

    _last_state = switch_state;
}


void GENERIC_TOOGLE::report_presence(bool presence)
{
    if (!_mqtt_client->isConnected())
    {
        return;
    }
    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/presence");
    _mqtt_client->publish(constructedTopic, presence ? "true" : "false", true);
    Serial.print("Reported presence: ");
    Serial.println(presence ? "true" : "false");
}

void GENERIC_TOOGLE::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    SwitchSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/luminosityThreshold");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           { 
                               _luminosity_threshold = atof(payload.c_str()); 
                               Serial.print("Luminosity threshold set to: ");
                               Serial.println(_luminosity_threshold);
                           });

    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/luminosityHysteresis");
    mqtt_client->unsubscribe(constructedTopic); // Unsubscribe from hysteresis topic

    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/disableNotifications");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           { 
                               disable_notifications = (payload == "true"); 
                               Serial.print("Disable notifications set to: ");
                               Serial.println(disable_notifications);
                           });

    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/notificationsCooldown");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           { 
                               _notifications_cooldown = atol(payload.c_str()); 
                               Serial.print("Notifications cooldown set to: ");
                               Serial.println(_notifications_cooldown);
                           });
}