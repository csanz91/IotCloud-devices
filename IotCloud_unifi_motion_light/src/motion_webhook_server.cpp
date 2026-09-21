#include "motion_webhook_server.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <cstring>

MotionWebhookServer::MotionWebhookServer(
    MotionLightSensor &motion_light,
    const char *webhook_token,
    const bool runtime_enabled)
    : _motion_light(motion_light),
      _webhook_token(webhook_token),
      _runtime_enabled(runtime_enabled)
{
    _server.collectHeaders(TOKEN_HEADER);
    _server.on(MOTION_PATH, HTTP_GET, [this]() { handle_motion(); });
    _server.on(HEALTH_PATH, HTTP_GET, [this]() {
        _server.send(200, "text/plain", "ok\n");
    });
    _server.onNotFound([this]() { handle_not_found(); });
}

void MotionWebhookServer::loop()
{
    if (!_runtime_enabled)
    {
        return;
    }

    if (station_is_connected())
    {
        if (!_started)
        {
            start();
        }
        _server.handleClient();
        return;
    }

    if (_started)
    {
        stop();
    }
}

void MotionWebhookServer::start()
{
    _server.begin();
    _started = true;
    Serial.print(F("UniFi webhook server listening at http://"));
    Serial.println(WiFi.localIP());
}

void MotionWebhookServer::stop()
{
    _server.stop();
    _started = false;
}

void MotionWebhookServer::handle_motion()
{
    if (!token_is_valid())
    {
        _server.send(401, "text/plain", "Unauthorized\n");
        return;
    }

    _motion_light.trigger_motion(millis());
    _server.send(204, "text/plain", "");
}

void MotionWebhookServer::handle_not_found()
{
    if (_server.uri() == MOTION_PATH || _server.uri() == HEALTH_PATH)
    {
        _server.sendHeader("Allow", "GET");
        _server.send(405, "text/plain", "Method Not Allowed\n");
        return;
    }

    _server.send(404, "text/plain", "Not Found\n");
}

bool MotionWebhookServer::token_is_valid() const
{
    if (!_server.hasHeader(TOKEN_HEADER))
    {
        return false;
    }

    const String supplied = _server.header(TOKEN_HEADER);
    const size_t expected_length = strlen(_webhook_token);
    size_t difference = supplied.length() ^ expected_length;

    for (size_t index = 0; index < expected_length; ++index)
    {
        const char supplied_character =
            index < supplied.length() ? supplied[index] : '\0';
        difference |= static_cast<unsigned char>(
            supplied_character ^ _webhook_token[index]);
    }

    return difference == 0;
}

bool MotionWebhookServer::station_is_connected() const
{
    const WiFiMode_t mode = WiFi.getMode();
    return (mode == WIFI_STA || mode == WIFI_AP_STA) &&
           WiFi.status() == WL_CONNECTED;
}
