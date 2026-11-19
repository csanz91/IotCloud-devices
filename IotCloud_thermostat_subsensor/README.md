# Madrid thermostat subsensor

## Device inventory

SI7021 temperature and humidity sensor with an SSD1306 OLED display. It reports to the separate Madrid thermostat.

- **Firmware model** `THERMOSTAT_SUBSENSOR`
- **Hardware** ESP8266. Custom NodeMCU build. Board vendor unknown.
- **Arduino build** [`nodemcuv2`](platformio.ini), configured version [`v1.1.3`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Unassigned | `18fe349d-6861` | Subsensor (analog_subsensor/temperature) | `v1.1.3` via MQTT | `1.3.0` | `v1.1.3` | Connected |

The [Madrid heating relay](../IotCloud_madrid_thermostat/README.md) consumes the remote readings from this device.

[Device inventory index](../README.md#device-inventory)
