#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include "chain_waxer_sensor.h"

class WaxerWebPortal
{
public:
    explicit WaxerWebPortal(ChainWaxerSensor &sensor);

    void begin();
    void loop();

private:
    static constexpr uint16_t HTTP_PORT = 80;

    ESP8266WebServer _server{HTTP_PORT};
    ChainWaxerSensor &_sensor;
    bool _started = false;

    void start();
    void stop();

    // Route handlers
    void handle_root();
    void handle_api_status();
    void handle_api_stage();
    void handle_api_target();
    void handle_api_timer_reset();
    void handle_api_timer_toggle();
    void handle_api_offset();
    void handle_not_found();

    bool is_wifi_ready() const;
};
