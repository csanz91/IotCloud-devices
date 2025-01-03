/* FastLED_RGBW
 *
 * Hack to enable SK6812 RGBW strips to work with FastLED.
 *
 * Original code by Jim Bumgardner (http://krazydad.com).
 * Modified by David Madison (http://partsnotincluded.com).
 * Modified by Christof Kaufmann.
 *
*/

// See: https://www.partsnotincluded.com/fastled-rgbw-neopixels-sk6812/
// See: https://www.dmurph.com/posts/2021/1/cabinet-light-3.html

#pragma once

#include "FASTLED_RGBW.h"



CRGB blend(CRGBW a, CRGB const& b) {
  return a.screen_blend(b);
}


/**
 * @brief Convert the RGB values to RGBW values in send order
 *
 * For details, see #CRGBWArray::convert_to_rgbw()
 */
void _rgb2rgbw(CRGBW leds[], size_t n_leds) {
  for (uint16_t idx = 0; idx < n_leds; ++idx) {
    CRGBW& led = leds[idx];
    if (led.w > 0)
      continue;

    // These values are what the 'white' value would need to be to get the corresponding color value.
    float white_val_from_red   = led.r * whitepoint_red_factor;
    float white_val_from_green = led.g * whitepoint_green_factor;
    float white_val_from_blue  = led.b * whitepoint_blue_factor;

    // Set the white value to the highest it can be for the given color
    // (without over saturating any channel - thus the minimum of them).
    float min_white_value = std::min(white_val_from_red, std::min(white_val_from_green, white_val_from_blue));
    // Don't use transformation for very dark values, since rounding errors are visible there
    if (min_white_value > 4) {
      // The rest of the channels will just be the original value minus the contribution by the white channel.
      led.r = static_cast<uint8_t>(std::round(led.r - led.w / whitepoint_red_factor));
      led.g = static_cast<uint8_t>(std::round(led.g - led.w / whitepoint_green_factor));
      led.b = static_cast<uint8_t>(std::round(led.b - led.w / whitepoint_blue_factor));
      led.w = static_cast<uint8_t>(std::floor(min_white_value));
    }

    // GRBW send order must be taken into account here and set outside to RGB
    std::swap(led.r, led.g);
  }
}



/**
 * @brief Convert RGBW values back to RGB
 *
 * This converts the RGBW values back to RGB in case you want to modify them
 * instead of completely overwriting them. It also accounts for the send order
 * by swapping back red and green values.
 */
void _rgbw2rgb(CRGBW leds[], size_t n_leds) {
  for (uint16_t idx = 0; idx < n_leds; ++idx) {
    CRGBW& led = leds[idx];

    // Reverse send order here
    CRGBW temp = led;
    std::swap(temp.r, temp.g);

    // Reverse transformation
    led.r = static_cast<uint8_t>(std::max(255.0f, std::round(temp.r + temp.w / whitepoint_red_factor)));
    led.g = static_cast<uint8_t>(std::max(255.0f, std::round(temp.g + temp.w / whitepoint_green_factor)));
    led.b = static_cast<uint8_t>(std::max(255.0f, std::round(temp.b + temp.w / whitepoint_blue_factor)));
    led.w = 0;
  }
}


