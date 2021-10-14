#ifndef _RGB_LAMP
#define _RGB_LAMP

#include "FastLED.h"

#include <sensors/led.h>

class RgbLamp : public LedSensor
{

    static constexpr int NUM_MAX_LEDS = 255;
    static constexpr int DATA_PIN = 1;

private:
    const int _num_leds;
    CRGB _leds[NUM_MAX_LEDS];
    void set_brightness(float brightness);
    void rainbow();

public:
    RgbLamp(
        const char *sensor_id,
        const char *sensor_name,
        const int num_leds);
    void loop();
};

#endif // _RGB_LAMP