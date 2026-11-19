# Legacy and unmatched firmware devices

**Registry and broker snapshot** 19 November 2026, 07:10 CET.

The Espruino script names below refer to the sibling `espruino/projects/` checkout. The registry model is blank for the five legacy devices. Their firmware version comes from the database. MQTT sessions establish current connectivity.

## Madrid Salon light relay

Probable Shelly 1 with an ESP8266-family MCU. The DB model is blank. The matching sensor suffix is `switch` in `shelly_salorn.js`. The [Arduino Shelly 1 project](../../IotCloud_shelly1/README) has no current registry match.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Madrid | Salon | `b4e62d55-444c` | Salon (light/light_switch) | `1.0` in DB | — | — | Connected. DB status still offline. |

## Teruel garage and garden gate

Custom ESP8266 NodeMCU relay controller. The `puerta_dentro.js` sketch matches the registered `garaje` and `jardin` sensor IDs. It pulses separate outputs for the two actuators. The Espruino garage provisioning file identifies the same device.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Teruel | Garaje | `18fe349b-1e5b` | Garaje (toggle/garage), Jardin (toggle/gate) | `1.0` in DB | — | — | Connected. DB status still offline. |

## Teruel Comedor thermostat

Legacy Espruino ESP8266 thermostat. The Espruino Comedor provisioning file identifies this database device. The exact board vendor and loaded JS revision are unverified. The repository contains an ESP8266 thermostat implementation in `termostato3.js`.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Teruel | Comedor | `5ccf7f2c-eaad` | Comedor (thermostat) | `1.0` in DB | — | — | Connected |

## Teruel outdoor weather station

Inferred ESP8266 NodeMCU and Espruino match. `estacion_mete.js` defines the same temperature, humidity and rainfall totalizer suffixes. The saved legacy provisioning file uses an older device ID. The current DB model is blank.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Teruel | Terraza | `5ccf7fd3-d9f8` | Hum. Ext (analog), Temp. Ext (analog), Totalizer (totalizer) | `1.0` in DB | — | `1.0` | Connected |

## Teruel Atico thermostat

Likely SONOFF TH-series hardware with an ESP8266-family chip. `termostato_atico.js` uses the SONOFF SI7021 interface and the matching `Thermostat` sensor suffix. Exact board revision and loaded JS revision are unverified.

| Location | DB room | DB display name | Registered sensors | Reported firmware | Arduino lib | OTA target | MQTT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Teruel | Atico | `bcddc29e-0cd8` | Atico (thermostat) | `1.0` in DB | — | `1.0` | Connected |

## Retired Teruel LD2410 devices

The database records for `bcddc224-8933` and `bcddc29e-1200` were removed on 19 November 2026. Both last reported `ESP8266_LD2410_TERUEL` firmware `v1.0.2`, and their latest recorded status events before deletion were from 30 December 2025. Both retained MQTT model messages were still visible after deletion. Neither device had a connected broker session.

[Device inventory index](../../README.md#device-inventory)
