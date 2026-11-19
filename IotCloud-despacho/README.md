# Despacho multisensor

## Device inventory

Office monitor with CO₂, temperature, humidity, presence and the Hexaled LED panel.

- **Firmware model** `Despacho`
- **Hardware** ESP8266. Custom NodeMCU build. Board vendor unknown.
- **Arduino build** [`nodemcuv2`](platformio.ini), configured version [`v1.0.19`](platformio.ini)

## OTA on this device

Despacho has only ~25 KB of free heap with all its sensors running, so it is the tightest ESP8266 device for OTA. With BearSSL's default 16 KB TLS receive buffer, ~4 KB was left after the handshake, so updates failed (400) or crashed with OOM. Because the update request is retained, a crash then meant a reboot loop. The IotCloud library now always uses 1 KB / 512 B TLS buffers with max fragment length (~20 KB left). It also refuses a version with 410 after it crashes the device twice. v1.0.7 to v1.0.18 were test builds for that fix. v1.0.10, v1.0.12 and v1.0.14 are marked rolled back on the server and can't be redeployed. v1.0.19 was installed over OTA on 19 November 2026.

## Hexaled wiring and FastLED pin numbering

- **LEDs** 8 hexagons × 20 RGBW pixels (WS2812-timed), driven through the RGBW hack in [`FASTLED_RGBW.h`](src/FASTLED_RGBW.h). The hack hands FastLED an RGB array 4/3 the size.
- **Data pin** NodeMCU **D5 (GPIO14)**, written as `addLeds<WS2812, D5, RGB>` in [`hexa_led.cpp`](src/hexa_led.cpp). `D5` expands to raw GPIO14, which matches FastLED's default raw numbering. The old Arduino IDE binary also used pin 14.
- **Don't use** `FASTLED_ESP8266_NODEMCU_PIN_ORDER` with a bare `5`. Up to v1.0.5, `hexa_led.h` defined it. With FastLED 3.10.9, setup switches GPIO14 to an output, but the bit-bang loop builds its mask from the bare number (`1u << 5`). So the data went to GPIO5, which is D1, the SI7021 pin, and the panel stayed dark. Fixed in v1.0.6.

**Registry and broker snapshot** 19 November 2026, 09:01 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Despacho | `2cf43279-f068` | CO2 (analog/co2), Hexaled (led), Hum. (analog/humidity), Pres. Desp (presence/presence), Temp. (analog/temperature) | `v1.0.19` in DB | `1.3.0` | `v1.0.19` | Connected |

[Device inventory index](../README.md#device-inventory)
