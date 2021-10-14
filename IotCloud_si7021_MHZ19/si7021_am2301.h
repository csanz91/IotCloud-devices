#pragma once

#include <sensors/analog.h>

class SI7021_AM2301
{
    enum ReadingErrors
    {
        error_no_reading,
        error_protocol_timeout,
        error_checksum_error,
        error_invalid_NAN_reading,
        no_error
    };

public:
    enum SensorModel
    {
        DHT11,
        DHT12,
        DHT22,
        AM2301,
        SI7021
    };
    float temperature;
    float humidity;

    void read_values();
    SI7021_AM2301(
        const int sensor_pin,
        const SensorModel sensor_model);

private:
    SensorModel _sensor_model;
    const int _sensor_pin;
    ReadingErrors _last_error;

    bool waitState(int);
    int read_dht_dat();
    ReadingErrors do_plugin_read();
    float get_temperature();
    float get_humidity();
};