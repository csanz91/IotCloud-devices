#pragma once

#include <sensors/analog.h>
#include <MHZ19.h>
#include <SoftwareSerial.h>

class IotCloud_CO2 : public AnalogSensor
{

private:
    SoftwareSerial * _serial;
    MHZ19 *_co2_sensor;
    void get_value();
    void init(char *mqtt_header, EspMQTTClient *mqtt_client);

public:
    IotCloud_CO2(
        const char *sensor_id,
        const char *sensor_name,
        const char *metadata,
        const int rx, const int tx);
};