#pragma once

#include <MHZ.h>

class MHZ19
{

public:
    float temperature;
    float ppm;

    void read_values();
    MHZ19(
        const int rx, const int tx);

private:
    const int _rx, _tx;
    MHZ *_co2_sensor;
};