# Athom power-monitoring plug

## Device inventory

Power-monitoring smart plug with a CSE7766 UART meter, relay, button and LED.

- **Firmware model** `IOTORETO_PLUG`
- **Hardware** ESP8285H16. Athom Plug V2, 2 MB flash.
- **Arduino build** [`esp8285_2m`](platformio.ini), configured version [`v1.0.12`](platformio.ini)

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Despacho | `007007fb-c921` | Smart Plug (smart_plug) | `v1.0.10` in DB | `1.1.0` | `v1.0.12` | Offline. Latest DB event 2026-11-08. |
| Madrid | Salon | `007007fb-dc33` | Impresora 3D (smart_plug) | `v1.0.12` via MQTT | `1.3.0` | `v1.0.12` | Connected |

The Despacho unit last reported `v1.0.10` and has target `v1.0.12`. The Salon 3D-printer plug reported `v1.0.12` over MQTT.

[Device inventory index](../README.md#device-inventory)
