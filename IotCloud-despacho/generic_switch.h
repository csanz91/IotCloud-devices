#pragma once
#include <sensors/switch.h>
#include <Wire.h>
#include <BH1750.h>
#include "notifier.h"

class GENERIC_TOOGLE : public SwitchSensor
{
private:
    const int _switch_pin;
    void set_state(bool);
    bool _last_state;
    bool _mem_switch_state_1 = false;
    unsigned long _last_debounce_time_1 = 0;
    unsigned long _setup_time = 0;
    const unsigned long START_DELAY = 5000;
    void report_presence(bool presence);
    void loop();

public:
    GENERIC_TOOGLE(
        const char *sensor_id,
        const char *sensor_name,
        const int switch_pin);

    void init(char *mqtt_header, EspMQTTClient *mqtt_client);
};