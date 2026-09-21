#pragma once

#include <ESP8266WebServer.h>

#include "motion_light_sensor.h"

class MotionWebhookServer
{
public:
    MotionWebhookServer(
        MotionLightSensor &motion_light,
        const char *webhook_token,
        bool runtime_enabled);
    void loop();

private:
    static constexpr uint16_t HTTP_PORT = 80;
    static constexpr const char *MOTION_PATH = "/webhooks/unifi/motion";
    static constexpr const char *HEALTH_PATH = "/healthz";
    static constexpr const char *TOKEN_HEADER = "X-Webhook-Token";

    ESP8266WebServer _server{HTTP_PORT};
    MotionLightSensor &_motion_light;
    const char *_webhook_token;
    const bool _runtime_enabled;
    bool _started = false;

    void start();
    void stop();
    void handle_motion();
    void handle_not_found();
    bool token_is_valid() const;
    bool station_is_connected() const;
};
