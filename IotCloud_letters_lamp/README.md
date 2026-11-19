# Letters lamp

## Device inventory

Addressable RGB letter lamp with rainbow effects and persistent on/off state.

- **Firmware model** `RGB_RAINBOW`
- **Hardware** ESP8266. Custom NodeMCU build. Board vendor unknown.
- **Arduino build** [`nodemcuv2`](platformio.ini), configured version [`v1.8`](platformio.ini)

## LED wiring and FastLED pin numbering

- **LEDs** 110 WS2812 pixels, GRB order, data on NodeMCU pin **D1 (GPIO5)**.
- **Code** `DATA_PIN = D1` in [`rgb_lamp.h`](src/rgb_lamp.h). The Arduino `D1` constant expands to raw GPIO5, which matches FastLED's default raw GPIO numbering.
- **Don't use** a bare `1` or `FASTLED_ESP8266_NODEMCU_PIN_ORDER`. Up to v1.7 (Arduino IDE build) the code said `DATA_PIN = 1`, which only meant D1 because the old core's `nodemcuv2` board turned on FastLED's NodeMCU pin order. Under PlatformIO it would drive GPIO1 (TX). See the [clock README](../IotCloud_7seg_clock/README.md#why-the-leds-went-dark-v141-to-v144) for the full story.

## History

- **v1.7** Arduino IDE sketch (`letters_lamp.ino`) on the old IotCloud library (`v0.4_arduino`).
- **v1.8** Moved to PlatformIO with the shared `nodemcuv2_base` and `ota_template`, IotCloud 1.3.0, latest FastLED, raw `D1`. Flashed over USB on 19 November 2026. Model and version now come from [`platformio.ini`](platformio.ini), so later updates can go over OTA (`pio run -t upload_remote`).

**Registry and broker snapshot** 19 November 2026, after the v1.8 flash.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Salon | `2cf43279-f454` | Yeah! (led/light_switch) | `v1.8` in DB | `1.3.0` | — | Connected |

The DB model field was blank before v1.8.

[Device inventory index](../README.md#device-inventory)
