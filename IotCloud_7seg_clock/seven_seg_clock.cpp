#include "seven_seg_clock.h"

const uint8_t Seven_Seg_Clock::char0[] = {4, 7, 10, 255};
const uint8_t Seven_Seg_Clock::char1[] = {0, 1, 3, 4, 6, 7, 9, 10, 12, 13, 255};
const uint8_t Seven_Seg_Clock::char2[] = {3, 4, 10, 11, 255};
const uint8_t Seven_Seg_Clock::char3[] = {3, 4, 9, 10, 255};
const uint8_t Seven_Seg_Clock::char4[] = {1, 4, 9, 10, 12, 13, 255};
const uint8_t Seven_Seg_Clock::char5[] = {4, 5, 9, 10, 255};
const uint8_t Seven_Seg_Clock::char6[] = {4, 5, 10, 255};
const uint8_t Seven_Seg_Clock::char7[] = {3, 4, 6, 7, 9, 10, 12, 13, 255};
const uint8_t Seven_Seg_Clock::char8[] = {4, 10, 255};
const uint8_t Seven_Seg_Clock::char9[] = {4, 9, 10, 12, 13, 255};

const uint8_t *Seven_Seg_Clock::_chars[10] = {char0, char1, char2, char3, char4, char5, char6, char7, char8, char9};

const CHSV Seven_Seg_Clock::_segBlack(0, 0, 0); // black

const TBlendType Seven_Seg_Clock::_current_blending = LINEARBLEND;
const CRGBPalette16 Seven_Seg_Clock::_current_palette = RainbowColors_p;

Seven_Seg_Clock::Seven_Seg_Clock(
    const char *sensor_id,
    const char *sensor_name) : LedSensor(sensor_id,
                                         sensor_name,
                                         "led",
                                         "{\"settings\":[{\"name\":\"Initial Brightness\",\"description\":\"Brightness for the device with the ON state.\",\"endpoint\":\"initialBrightness\",\"actionType\":\"floatpar\",\"value\":0.5,\"limits\":{\"hi\":1.0,\"lo\":0.0}}]}")
{
    FastLED.addLeds<WS2811, _data_pin, GRB>(_leds, _num_leds)
        .setCorrection(Typical8mmPixel);

    calculate_digits();
    FastLED.clear();
    FastLED.show();
}

void Seven_Seg_Clock::calculate_digits()
{
    time_t now;
    now = time(nullptr);
    struct tm *tmp = localtime(&now);
    _digits = (tmp->tm_hour * 100 + tmp->tm_min);
}

void Seven_Seg_Clock::set_segments(uint8_t color_index)
{
    CHSV segCOLOR(0, 0, 0);

    for (int i = 0; i < _num_leds; i++)
    {
        _leds[i] = ColorFromPalette(_current_palette, color_index, _brightness * 255, _current_blending);
        color_index++;

        const uint8_t seg = i % _num_leds_per_segment;
        const uint8_t seg_digit = (_num_leds - i - 1) / _num_leds_per_segment;
        const uint8_t digit_to_draw = (_digits / (int)pow(10, seg_digit)) % 10;

        for (int x = 0; x < _num_leds_per_segment; x++)
        {
            const uint8_t seg_idx = _chars[digit_to_draw][x];
            if (seg_idx == 255)
            {
                break;
            }
            if (seg_idx == seg)
            {
                _leds[i] = _segBlack;
                break;
            }
        }
    }
}

void Seven_Seg_Clock::loop()
{
    if (state)
    {
        EVERY_N_MILLISECONDS(70)
        {
            _color_index++;
        }
        set_segments(_color_index);
    }
    else
        FastLED.clear();

    EVERY_N_MILLISECONDS(5000)
    {
        calculate_digits();
    }
    FastLED.show();
}