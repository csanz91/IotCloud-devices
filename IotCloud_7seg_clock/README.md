# Seven-segment LED clock

## Device inventory

Four-digit addressable LED clock with NTP synchronization and a rainbow color palette.

- **Firmware model** `7SEG_CLOCK`
- **Hardware** ESP8266. Custom NodeMCU build. Board vendor unknown.
- **Arduino build** [`nodemcuv2`](platformio.ini), configured version [`v1.4.6`](platformio.ini)

## LED wiring and FastLED pin numbering

- **LEDs** 60 WS2811 pixels (4 digits × 15), GRB order, data on NodeMCU pin **D1 (GPIO5)**.
- **Code** `_data_pin = D1` in [`seven_seg_clock.h`](src/seven_seg_clock.h). The Arduino `D1` constant expands to raw GPIO5, which matches FastLED's default raw GPIO numbering.
- **Don't use** `FASTLED_ESP8266_NODEMCU_PIN_ORDER` with bare numbers like `1`.

### Why the LEDs went dark (v1.4.1 to v1.4.4)

The code used to say `_data_pin = 1`, meaning NodeMCU label D1. That only works under FastLED's NodeMCU pin order:

- **v1.4.1 (first PlatformIO build)** FastLED turns on NodeMCU order only when the board defines `ARDUINO_ESP8266_NODEMCU`. PlatformIO's `nodemcuv2` defines `ARDUINO_ESP8266_NODEMCU_ESP12E`, so FastLED used raw numbering and drove GPIO1. That is the TX pin, which `Serial.begin()` takes. Dark.
- **v1.4.2** pinned FastLED 3.10.3. Still dark.
- **v1.4.3** added `-DFASTLED_ESP8266_NODEMCU_PIN_ORDER` (with FastLED 3.10.3). Worked.
- **v1.4.4** same flag with FastLED 3.10.9. Its ESP8266 bit-bang loop builds the GPIO mask from the bare template pin (`1u << pin` in `clockless_esp8266.cpp.hpp`) and ignores the NodeMCU mapping, so the data went to GPIO1 again.
- **v1.4.5** raw `D1` and no pin-order flag. Works on any FastLED version.

**Registry

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Salon | `2cf43251-230e` | Clock (led/clock) | `v1.4.1` via MQTT | `1.3.0` | `v1.4.1` | Connected |

Since the snapshot: v1.4.3 restored the LEDs. v1.4.5 (latest FastLED, raw `D1`) was confirmed working, and v1.4.6 (same firmware with the IotCloud OTA memory fix) was installed over OTA on 19 November 2026.

[Device inventory index](../README.md#device-inventory)
