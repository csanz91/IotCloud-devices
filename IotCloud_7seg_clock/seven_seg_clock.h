#ifndef _Seven_Seg_Clock
#define _Seven_Seg_Clock

#include "FastLED.h"
#include <sensors/led.h>

class Seven_Seg_Clock : public LedSensor
{

    static constexpr int _data_pin = 1;
    static constexpr int _num_leds = 60;

private:
    CRGBArray<_num_leds> _leds;
    static const CHSV _segBlack; //black

    const int _num_leds_per_segment = 15;

    static const uint8_t char0[];
    static const uint8_t char1[];
    static const uint8_t char2[];
    static const uint8_t char3[];
    static const uint8_t char4[];
    static const uint8_t char5[];
    static const uint8_t char6[];
    static const uint8_t char7[];
    static const uint8_t char8[];
    static const uint8_t char9[];

    static const uint8_t *_chars[10];

    int _digits = 8888;
    uint8_t _color_index = 0;

    static const TBlendType _current_blending;
    static const CRGBPalette16 _current_palette;
    //  currentPalette = CloudColors_p;
    //  currentPalette = PartyColors_p;
    //  currentPalette = ForestColors_p;
    //  currentPalette = OceanColors_p;

    void calculate_digits();
    void set_segments(uint8_t);

public:
    Seven_Seg_Clock(
        const char *sensor_id,
        const char *sensor_name);
    void loop();
};

#endif // _Seven_Seg_Clock