#ifndef _HEXA_LED
#define _HEXA_LED

#include <sensors/led.h>
#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#include <FastLED.h>
#include "FastLED_RGBW.h"

class HexaLed : public LedSensor
{

    static constexpr int _num_hexagons = 8;
    static constexpr int _num_leds_per_hexagon = 20;
    static constexpr int _num_leds = _num_hexagons * _num_leds_per_hexagon;

private:
    CRGBWArray<_num_leds> _leds;
    void set_brightness(float brightness);
    void rainbow();
    uint8_t _hue = 0;
    unsigned int _lastTask = 0;

public:
    HexaLed(
        const char *sensor_id,
        const char *sensor_name);
    void loop();
};

#endif // _HEXA_LED