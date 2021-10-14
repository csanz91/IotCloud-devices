#include <FastLED.h>
#include <memory> // for std::array in CRGBWArray

/**
 * @brief Extends CRGB by white
 *
 * This contains RGB values and additionally a value for white. This allows to
 * use RGBW LEDs with FastLED, but not perfectly. Things like color or
 * temperature correction won't work, since it is applied right before sending.
 * But this workaround does not give FastLED real RGBW capabilities, so to be
 * clear: FastLED still thinks that it is sending color data to RGB LEDs:
 * @code
 * FastLED sees:  R G B|R G B|R G B|R G B|R G B|R G B|
 * True LEDs are: R G B W|R G B W|R G B W|R G B W|
 * @endcode
 */
struct CRGBW : public CRGB
{
    union
    {
        uint8_t w;
        uint8_t white;
    };

    CRGBW() : CRGB(0, 0, 0), w{0} {}

    CRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0)
        : CRGB(r, g, b), w{w}
    {
    }

    inline void operator=(const CRGB c) __attribute__((always_inline))
    {
        this->r = c.r;
        this->g = c.g;
        this->b = c.b;
        this->w = 0;
    }

    /**
   * @brief Blend two colors in screen mode
   * 
   * This blends two colors nicely giving a bright color.
   * Alpha is not supported here, since CRGBW does not support it.
   * 
   * @see https://en.wikipedia.org/wiki/Blend_modes
   */
    CRGBW &screen_blend(CRGB b)
    { // parameter type must be `CRGB` instead of `CRGB const&` due to a bug in operator-
        CRGB complement = -(*this);
        *this = -(complement.scale8(-b));
        return *this;
    }
};

/**
 * @brief Number of RGB LEDs required to contain the RGBW LEDs values
 *
 * For details, see #CRGBWArray::rgb_size()
 */
constexpr int rgb_size_from_rgbw_size(int nleds_rgbw)
{ // original name: getRGBWsize
    return nleds_rgbw * 4 / 3 + (nleds_rgbw % 3 != 0);
}

using CRGBWSet = CPixelView<CRGBW>;

__attribute__((always_inline)) inline CRGBW *operator+(CRGBWSet const &pixels, int offset)
{
    return (CRGBW *)pixels + offset;
}

/**
 * @brief Calibration factors of the white LED
 *
 * The values here mean if white is set to 100 (and red, green and blue are
 * off), it has the same color and brightness as if red is set to 255, green to
 * 140 and blue to 25 (and white is off).
 *
 * To find these values, start by trying to match the color of white and then
 * brightness. The brightness is hard to estimate, so looking at converted
 * colors might help.
 */
constexpr float whitepoint_red_factor = 100. / 255.;
constexpr float whitepoint_green_factor = 100. / 140.; ///< @copydoc whitepoint_red_factor
constexpr float whitepoint_blue_factor = 100. / 25.;   ///< @copydoc whitepoint_red_factor

/**
 * @brief CRGBW Array class
 *
 * Here is an example of how to use it. First, create the array as global
 * variable:
 * @code
 * constexpr int NUM_LEDS = 87;
 * CRGBWArray<NUM_LEDS> leds;
 * @endcode
 *
 * Then, configure FastLED to use it (e. g. in the Arduino `setup()` function):
 * @code
 * constexpr int LED_IO = 19;
 * FastLED.addLeds<SK6812, LED_IO, RGB>(leds, leds.rgb_size());
 * @endcode
 * Note, that the send order must be RGB, although the real send order is GRBW!
 * After drawing your effects into `leds`, you have to convert the colors to
 * RGBW, which also handles the send order (e. g. in the Arduino `loop()`
 * function):
 * @code
 * leds.convert_to_rgbw();
 * FastLED.delay(100); // send during delay
 * @endcode
 */
template <int SIZE>
class CRGBWArray : public CPixelView<CRGBW>
{
    std::array<CRGBW, SIZE> rawleds;

public:
    CRGBWArray() : CPixelView<CRGBW>(rawleds.data(), SIZE) {}
    using CPixelView::operator=;

    /**
     * @brief Number of RGB LEDs required to contain the RGBW LEDs values
     *
     * This is useful for the `FastLED.addLeds()` call, see the example at
     * #CRGBWArray.
     *
     * Example: For 10 RGBW LEDs you need 40 bytes color data, which fits into
     * 14 RGB LEDs (42 bytes).
     */
    static constexpr int rgb_size()
    {
        return rgb_size_from_rgbw_size(SIZE);
    }

    /**
     * @brief Convert this Array to a CRGB pointer onto the first element
     *
     * This is useful for the `FastLED.addLeds()` call.
     */
    inline operator CRGB *()
    {
        return reinterpret_cast<CRGB *>(rawleds.data());
    }

    /**
     * @brief Convert the RGB values to RGBW values in send order
     *
     * The part in the RGB values, that can be represented by the white led is
     * removed from the RGB values and set to the white value.
     * 
     * Using the equivalent RGBW values reduces power consumption and enhances
     * the light quality (RA value). However, it does not allow to go beyond
     * the brightness of full RGB. You could set the white value afterwards if
     * you really want.
     *
     * Besides it swaps the red and green channel in the converted values to
     * handle the send order.
     */
    inline void convert_to_rgbw()
    {
        rgb2rgbw(*this);
    }

    /**
     * @brief Convert the RGBW values in send order back to RGB values
     *
     * This swaps the red and green channel to revert the send order fix and
     * then adds the white led value split into RGB color components back to
     * the remained RGB values. This sets the white value to 0.
     * 
     * This function is useful, if you want to modify the led values directly
     * in the CRGBWArray.
     */
    inline void revert_to_rgb()
    {
        rgbw2rgb(*this);
    }
};

void _rgb2rgbw(CRGBW leds[], size_t n_leds);
/// @copydoc rgb2rgbw
template <class LEDArray>
void rgb2rgbw(LEDArray &leds)
{
    _rgb2rgbw(&leds[0], leds.size());
}
/// @copydoc rgbw2rgb
template <class LEDArray>
void rgbw2rgb(LEDArray &leds)
{
    _rgbw2rgb(&leds[0], leds.size());
}

void _rgbw2rgb(CRGBW leds[], size_t n_leds);
CRGB blend(CRGBW a, CRGB const &b);