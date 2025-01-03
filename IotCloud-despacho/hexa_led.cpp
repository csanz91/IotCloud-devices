#include "hexa_led.h"

HexaLed::HexaLed(
    const char *sensor_id,
    const char *sensor_name) : LedSensor(sensor_id,
                                         sensor_name)
{
    // FastLED converts NODEMCU pins into the corresponding GPIO pin
    // 5 is the D5 pin
    FastLED.addLeds<WS2812, 5, RGB>(_leds, _leds.rgb_size());
}

void HexaLed::set_brightness(float brightness)
{
    LedSensor::set_brightness(brightness);
    FastLED.setBrightness(brightness * 255);
}

void HexaLed::rainbow()
{
    unsigned int elapsed = millis() - _lastTask;

    if (elapsed > 150)
    {
        // static uint16_t sPseudotime = 0;
        // static uint16_t sLastMillis = 0;
        // static uint16_t sHue16 = 0;

        // uint8_t sat8 = beatsin88(87, 220, 250);
        // uint8_t brightdepth = beatsin88(341, 46, 140);
        // uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
        // uint8_t msmultiplier = 15;

        // uint16_t hue16 = sHue16; //gHue * 256;
        // uint16_t hueinc16 = beatsin88(113, 1, 3000);

        // uint16_t ms = millis();
        // uint16_t deltams = ms - sLastMillis;
        // sLastMillis = ms;
        // sPseudotime += deltams * msmultiplier;
        // sHue16 += deltams * beatsin88(400, 2, 5);
        // uint16_t brightnesstheta16 = sPseudotime;

        // for (uint16_t i = 0; i < _num_hexagons; i++)
        // {
        //     hue16 += hueinc16;
        //     uint8_t hue8 = hue16 / 256;

        //     brightnesstheta16 += brightnessthetainc16;
        //     uint16_t b16 = sin16(brightnesstheta16) + 32768;

        //     uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
        //     uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
        //     bri8 += (255 - brightdepth);

        //     //fill_gradient(&_leds[i * _num_leds_per_hexagon], _num_leds_per_hexagon - 1, CHSV(hue8, sat8, bri8), CHSV(hue8, sat8, bri8));
        // }

        // const int offset = 255 / _num_hexagons;
        // for (size_t i = 0; i < _num_hexagons; i++)
        // {
        //     const int hex_hue = (i * offset) + _hue;
        //     fill_gradient(&_leds[i * _num_leds_per_hexagon], _num_leds_per_hexagon - 1, CHSV(hex_hue, 230, 255), CHSV(hex_hue + 10, 230, 255));
        // }
        fill_gradient(&_leds[0], _num_leds - 1, CHSV(_hue, 255, 255), CHSV(_hue +70, 255, 255));
        ++_hue;

        // prepare to send
        _leds.convert_to_rgbw();
        _lastTask = millis();
    }
}

void HexaLed::loop()
{
    if (state)
        rainbow();
    else
        FastLED.clear();

    FastLED.show();
}