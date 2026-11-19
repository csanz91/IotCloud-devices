# Madrid outdoor temperature sensor

## Device inventory

Outdoor DS18B20 temperature probe on D1 (GPIO5).

- **Firmware model** `MAD_WEATHER_STATION`
- **Hardware** ESP8266. Custom NodeMCU build. Board vendor unknown.
- **Arduino build** [`nodemcuv2`](platformio.ini), configured version [`v1.0.2`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Unassigned | `2cf43279-f968` | Temp. Ext (analog/temperature) | `v1.0.2` via MQTT | `1.3.0` | `v1.0.2` | Connected |

This is the same Madrid registry entry that reported model `DS18B20` before the migration. The Teruel outdoor weather station runs separate Espruino firmware.

[Device inventory index](../README.md#device-inventory)
