# IotCloud-devices

A comprehensive collection of Arduino/ESP-based IoT device implementations using the IotCloud library ecosystem. This repository contains ready-to-use firmware examples for various sensor and actuator projects that integrate seamlessly with the IotCloud platform.

## Table of Contents

- [Overview](#overview)
- [Repository Structure](#repository-structure)
- [Device inventory](#device-inventory)
  - [Firmware projects without a confirmed deployment](#firmware-projects-without-a-confirmed-deployment)
- [Device Projects](#device-projects)
  - [Environmental Sensors](#environmental-sensors)
  - [Smart Switches & Relays](#smart-switches--relays)
  - [LED Controllers](#led-controllers)
  - [Thermostats](#thermostats)
  - [Specialty Devices](#specialty-devices)
- [Getting Started](#getting-started)
- [Architecture](#architecture)
- [Dependencies](#dependencies)
- [Common Patterns](#common-patterns)
- [Configuration](#configuration)

---

## Overview

This repository provides production-ready firmware implementations for various IoT devices that connect to the IotCloud MQTT broker. Each device project follows a consistent architecture pattern:

1. **Device Declaration** - Creates a `Device` instance with version and type information
2. **Sensor Registration** - Adds one or more sensors inheriting from base sensor classes
3. **IotCloud Integration** - Uses `iotcloud_setup()` and `iotcloud_loop()` for connectivity

### Supported Platforms

| Platform | Description |
|----------|-------------|
| **ESP8266** | NodeMCU, Sonoff Basic, Sonoff TH10 |
| **ESP32** | ESP32 DevKit, ESP32-S2, ESP32-C3 |
| **Espruino** | JavaScript-based microcontrollers (via JS implementation) |

---

## Repository Structure

```
IotCloud-devices/
├── IotCloud_7seg_clock/         # 7-segment LED clock display
├── IotCloud_8button_keypad/     # Bedroom keypad and BLE scanner
├── IotCloud_ds18b20/            # DS18B20 temperature sensor
├── IotCloud_iotoreto_plug/      # Athom power-monitoring smart plug
├── IotCloud_ld2410/             # LD2410 radar presence sensor
├── IotCloud_ld2410_TH10/        # LD2410 for Sonoff TH10
├── IotCloud_ld2410_light/       # LD2410 presence and BH1750 illuminance
├── IotCloud_ld2410_salon_central/ # Salon Central LD2410 and light relay
├── IotCloud_letters_lamp/       # RGB letter lamp controller
├── IotCloud_puerta_principal/   # Teruel main door controller
├── IotCloud_shelly1/            # Shelly 1 Arduino relay firmware
├── IotCloud_si7021/             # SI7021 temperature/humidity sensor
├── IotCloud_sonoff_dual_r2/     # Sonoff Dual R2 blinds controller
├── IotCloud_sonoff_dual_r3/     # Sonoff Dual R3 (blinds control)
├── IotCloud_sonoff_dual_r3_madrid/  # Sonoff Dual R3 variant (2 switches, presence, BLE scanner)
├── IotCloud_sonoff_dual_r3_lite_ble/  # Sonoff Dual R3 Lite: BLE presence scanner + 2 switches
├── IotCloud_sonoff_dual_r3_lite_thermostat/  # Sonoff Dual R3 Lite: DS18B20 thermostat, relay 2 as dry contact
├── IotCloud_sonoff_plug/        # Sonoff S26 smart plug
├── IotCloud_sonoff_rf/          # Sonoff RF controller
├── IotCloud_thermostat/         # DS18B20-based thermostat
├── IotCloud_madrid_thermostat/  # Madrid thermostat relay (separate temperature sensor)
├── IotCloud_thermostat_subsensor/  # Thermostat subsensor
├── IotCloud_unifi_motion_light/ # UniFi Protect motion-triggered light
├── IotCloud_washing_machine/    # Washing machine monitor
├── IotCloud-despacho/           # Multi-sensor office device
├── shared/libs/IotCloudBleScanner/  # BLE presence scanner sensor, shared by devices
├── tools/ota_size_check.py     # OTA image size check (used by add_ota_target.py)
├── docs/legacy-devices/        # Espruino and unmatched device inventory
└── README.md
```

---

## Device inventory

**Registry and broker snapshot** 19 November 2026, 09:07 CET. IotCloud had 29 registered physical devices. Of these, 26 were directly connected (15 Madrid, 7 Teruel, 4 Zaragoza) and 3 were offline, all in Madrid. The database had 44 device records in total, including integrations, data feeds, the demo and monitoring records.

Each firmware README gives the board target, manufacturer where known, build and reported versions, database display names, rooms and sensors. The firmware configured in the source can differ from the version a device reports. For live status, use the broker session. Retained topics and historical database events can outlive a connection.

### Arduino and PlatformIO projects

| Project documentation | Firmware model | MCU | Location and DB display name |
| --- | --- | --- | --- |
| [Despacho multisensor](IotCloud-despacho/README.md) | `Despacho` | ESP8266 | Madrid `2cf43279-f068` |
| [Seven-segment LED clock](IotCloud_7seg_clock/README.md) | `7SEG_CLOCK` | ESP8266 | Madrid `2cf43251-230e` |
| [Bedroom eight-button keypad](IotCloud_8button_keypad/README.md) | `ESP32_8Button_Keypad` | ESP32-C3 | Madrid `Bedroom Keypad Device` |
| [Madrid outdoor temperature sensor](IotCloud_ds18b20/README.md) | `MAD_WEATHER_STATION` | ESP8266 | Madrid `2cf43279-f968` |
| [Athom power-monitoring plug](IotCloud_iotoreto_plug/README.md) | `IOTORETO_PLUG` | ESP8285H16 | Madrid `007007fb-c921` (offline), `007007fb-dc33` |
| [Baño LD2410 light controller](IotCloud_ld2410/README) | `ESP8266_LD2410` | ESP8266 family | Madrid `2cf43214-6d41` |
| [Cocina LD2410 light controller](IotCloud_ld2410_TH10/README) | `ESP8266_LD2410_TH10` | ESP8285 | Madrid `807d3a32-e1a5` |
| [Salon LD2410 and light sensor](IotCloud_ld2410_light/README) | `ESP8266_LD2410_LIGHT` | ESP8266 | Madrid `bcddc224-879f` |
| [Salon Central LD2410 light controller](IotCloud_ld2410_salon_central/README) | `ESP8266_LD2410_SALON_CENTRAL` | ESP8266 | Madrid `2cf4327a-732c` |
| [Yeah! RGB letter lamp](IotCloud_letters_lamp/README.md) | `RGB_RAINBOW` | ESP8266 | Madrid `2cf43279-f454` (inferred) |
| [Madrid heating relay](IotCloud_madrid_thermostat/README.md) | `MADRID_SUBSENSOR_THERMOSTAT` | ESP8266 | Madrid `cc50e30a-1d4c` |
| [Teruel main door controller](IotCloud_puerta_principal/README.md) | `PUERTA_PRINCIPAL` | ESP8266 | Teruel `cc50e356-892d` |
| [Teruel SONOFF Dual R2 blinds controller](IotCloud_sonoff_dual_r2/README) | `SONOFF_DUAL_R2` | ESP8285 | Teruel `dc4f2297-577e` |
| [Zaragoza SONOFF Dual R3 blinds controllers](IotCloud_sonoff_dual_r3/README.md) | `SONOFF_DUAL_R3` | ESP32 | Zaragoza `b8d61a62-ef50`, `b8d61a62-efa8`, `b8d61a66-4d90`, `b8d61a66-687c` |
| [Madrid SONOFF Dual R3 lights and presence](IotCloud_sonoff_dual_r3_madrid/README.md) | `SONOFF_DUAL_R3_MADRID` | ESP32 | Madrid `2415d6d6-cdc0` |
| [SONOFF smart plug](IotCloud_sonoff_plug/README.md) | `SONOFF_PLUG` | ESP8266 family | Madrid `84f3ebb0-d899` (offline) |
| [SONOFF RF Bridge R2](IotCloud_sonoff_rf/README) | `SONOFF_RF` | ESP8285 | Madrid `4cebd6f7-28d8` |
| [Teruel bedrooms thermostat](IotCloud_thermostat/README.md) | `ESP32_DS18B20_THERMOSTAT` | ESP32 | Teruel `20d74dbf-713c` |
| [Madrid thermostat subsensor](IotCloud_thermostat_subsensor/README.md) | `THERMOSTAT_SUBSENSOR` | ESP8266 | Madrid `18fe349d-6861` |
| [Washing machine notifier and start button](IotCloud_washing_machine/README.md) | `WASHING_MACHINE` | ESP8266 | Madrid `bcddc224-98c8` (offline) |

### Firmware projects without a confirmed deployment

Five of the 25 firmware projects have no matching device registry entry or MQTT model announcement. The OTA column shows files present on the server. The separate Madrid Salon relay runs Espruino on probable Shelly 1 hardware.

| Project documentation | Firmware model | MCU | OTA images | Firmware source |
| --- | --- | --- | --- | --- |
| [Shelly 1 relay](IotCloud_shelly1/README) | `SHELLY1` | ESP8285 | — | [IotCloud_shelly1.cpp](IotCloud_shelly1/src/IotCloud_shelly1.cpp) |
| [SI7021 temperature and humidity sensor](IotCloud_si7021/README.md) | `SI7021` | ESP8266 | — | [IotCloud_si7021.ino](IotCloud_si7021/IotCloud_si7021.ino) |
| [SONOFF Dual R3 Lite BLE scanner](IotCloud_sonoff_dual_r3_lite_ble/README.md) | `SONOFF_DUAL_R3_LITE_BLE` | ESP32 | `v1.1.0` | [IotCloud_sonoff_dual_r3_lite_ble.cpp](IotCloud_sonoff_dual_r3_lite_ble/src/IotCloud_sonoff_dual_r3_lite_ble.cpp) |
| [SONOFF Dual R3 Lite thermostat](IotCloud_sonoff_dual_r3_lite_thermostat/README.md) | `SONOFF_DUAL_R3_LITE_THERMOSTAT` | ESP32 | 7 images, latest `v1.2.1` | [IotCloud_sonoff_dual_r3_lite_thermostat.cpp](IotCloud_sonoff_dual_r3_lite_thermostat/src/IotCloud_sonoff_dual_r3_lite_thermostat.cpp) |
| [UniFi motion-triggered light](IotCloud_unifi_motion_light/README.md) | `NODEMCU_UNIFI_MOTION_LIGHT` | ESP8266 | — | [IotCloud_unifi_motion_light.cpp](IotCloud_unifi_motion_light/src/IotCloud_unifi_motion_light.cpp) |

### Legacy and unmatched devices

| Device documentation | DB display name | MQTT |
| --- | --- | --- |
| [Madrid Salon light relay](docs/legacy-devices/README.md#madrid-salon-light-relay) | `b4e62d55-444c` | Connected |
| [Teruel garage and garden gate](docs/legacy-devices/README.md#teruel-garage-and-garden-gate) | `18fe349b-1e5b` | Connected |
| [Teruel Comedor thermostat](docs/legacy-devices/README.md#teruel-comedor-thermostat) | `5ccf7f2c-eaad` | Connected |
| [Teruel outdoor weather station](docs/legacy-devices/README.md#teruel-outdoor-weather-station) | `5ccf7fd3-d9f8` | Connected |
| [Teruel Atico thermostat](docs/legacy-devices/README.md#teruel-atico-thermostat) | `bcddc29e-0cd8` | Connected |

### Integration and data records

These entries are represented in the database. Adapter connectivity describes the integration session.

| Location | DB display name | DB model | Connection or source |
| --- | --- | --- | --- |
| Demo Home | `Virtual Smart Hub` | `Virtual Device Suite` | Automation Runner MQTT adapter |
| Health Check | `healthcheck` | — | Synthetic monitoring record |
| Madrid | `Bedroom Automation` | `Embedded Virtual Switches` | Automation Runner MQTT adapter |
| Madrid | `Electricity` | `energy` | Energy record without sensors |
| Madrid | `Electricity` | `energy` | Energy data feed |
| Madrid | `Home Alone` | `Embedded Virtual Switches` | Automation Runner MQTT adapter |
| Madrid | `Madrid` | `Embedded Virtual Lights` | Automation Runner MQTT adapter |
| Madrid | `Madrid Automations` | `Embedded Virtual Switches` | Automation Runner MQTT adapter |
| Madrid | `Madrid Vacuum` | `Roborock V1` | Automation Runner MQTT adapter |
| Madrid | `Pi-hole Control` | `Embedded Virtual Pi-hole` | Automation Runner MQTT adapter |
| Madrid | `Power Meter` | `Embedded Virtual Power Meter` | Automation Runner MQTT adapter |
| Madrid | `weather_6c2cfb30-3260-11e9-800d-75a29fb7366b` | `weather` | AEMET data feed |
| Teruel | `Garden door camera` | `camera_door` | Automation Runner MQTT adapter |
| Teruel | `Teruel` | — | Registry record without sensors or a live session |
| Teruel | `weather_3466d970-0b50-11e9-8205-a19b97cb10a7` | `weather` | AEMET data feed |

### Removed devices and firmware projects

- **Teruel LD2410 devices** `bcddc224-8933` and `bcddc29e-1200` were removed from the device registry on 19 November 2026. Retained MQTT model messages were still visible. [Historical details](docs/legacy-devices/README.md#retired-teruel-ld2410-devices)

- **Removed standalone projects** MH-Z19 CO₂ monitor (`CO2`), Hexaled panel (`HEXALED`), rotary encoder (`Rotary`), SI7021 + MH-Z19 (`SI7021_CO2`) and SI7021 with OLED (`SI7021_OLED`). Despacho still uses its own CO₂ sensor and Hexaled panel. The OTA server still has Rotary images `v1.0.1`, `v1.0.2` and `v1.0.3`.
- **HotPot** used model `NODEMCU_CHAIN_WAXER` in Madrid. The database row and live MQTT session were absent. Old retained metadata was still visible.
- **Standalone ESP32 LD2410** used model `ESP32_LD2410`. The local firmware project was removed. No matching database record or MQTT model announcement was found.
- **SONOFF Mini** used firmware model `SONOFF_MINI`. The local firmware project was removed after checking the device registry and MQTT. The OTA server still has `v1.3.0` and `v1.3.1`, and the release table records `v1.3.1`.

## Device Projects

### Environmental Sensors

#### IotCloud_si7021
Temperature and humidity monitoring using the SI7021 sensor.

```cpp
Device my_device = Device("v1.2", "SI7021");
my_device.add_sensor(new IotCloud_T_H("001_T", "Temperature", metadata, &sensor.temperature));
my_device.add_sensor(new IotCloud_T_H("001_H", "Humidity", metadata, &sensor.humidity));
```

**Features:**
- SI7021 high-precision temperature/humidity sensor
- WiFi signal strength reporting
- Configurable polling interval (5 seconds default)
- Value filtering for noise reduction

#### IotCloud_ds18b20
Waterproof temperature sensor for outdoor/harsh environments.

```cpp
Device my_device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
my_device.add_sensor(new IOTCLOUD_DS18B20("001_T", "Temperature", ONE_WIRE_BUS));
```

**Features:**
- OneWire protocol support
- 12-bit resolution
- Suitable for pipe/tank monitoring

---

### Smart Switches & Relays

#### IotCloud_sonoff_dual_r3
Dual-relay controller for motorized blinds/shades.

```cpp
my_device.add_sensor(new SonoffDualR3Blinds("SonoffDualR3", "Persiana", RELAY_1, RELAY_2, SWITCH_1, SWITCH_2));
```

**Features:**
- ESP32 Dual R3, OTA only (default partitions), model `SONOFF_DUAL_R3`
- Same MQTT API as the Dual R2 blinds (`aux/setMovement` partial moves, `aux/movement` status)
- Wall switches move the blind while held (60 s cap), with noise-hardened debounce
- Reversal dead time, and a lockout that breaks input-noise feedback loops
- Host unit tests: `pio test -e native`

#### IotCloud_sonoff_dual_r3_lite_ble
Sonoff Dual R3 Lite firmware for an entryway scanner. It listens for phones' BLE presence
beacons and forwards per-token RSSI to the backend, which decides enter/leave.
Also keeps both relays as normal switches.

```cpp
my_device.add_sensor(new GENERIC_TOOGLE("Switch1", "Switch1", RELAY_1, SWITCH_1));
my_device.add_sensor(new GENERIC_TOOGLE("Switch2", "Switch2", RELAY_2, SWITCH_2));
my_device.add_sensor(&ble_scanner); // BleScannerSensor("001_BLE", ...), type ble_scanner
```

**Features:**
- Passive scan for beacon protocol v1 (holds no secrets), driving the BT
  controller over HCI with no BLE host stack
- Publishes `aux/ble/sighting` every 2 s per token, plus `gone` after 20 s
- `min_spiffs` partitions: the first flash must be over serial
- Host unit tests: `pio test -e native`

The scanner lives in `shared/libs/IotCloudBleScanner` and also runs on
`IotCloud_sonoff_dual_r3_madrid` (default partitions, OTA only) and
`IotCloud_8button_keypad` (`min_spiffs`). The three projects use the same scanner library.

#### IotCloud_ld2410
Radar-based presence detection for automatic light control.

```cpp
Device my_device = Device("v1.0.1", "ESP8266_LD2410");
my_device.add_sensor(new GENERIC_TOOGLE("BathroomLight", "Switch", RELAY_PIN, SWITCH_PIN));
```

**Features:**
- LD2410 mmWave radar integration
- Human presence detection
- Automatic relay control
- 5-second startup delay to avoid false triggers

---

### LED Controllers

#### IotCloud_letters_lamp
RGB letter sign with animated rainbow effect.

```cpp
Device my_device = Device("v1.7", "RGB_RAINBOW");
my_device.add_sensor(new RgbLamp("001", "Yeah!", NUM_LEDS));
```

**Features:**
- Up to 255 addressable LEDs
- Dynamic beatsin-based rainbow animation
- Smooth color blending
- On/off state persistence

#### IotCloud_7seg_clock
LED-based seven-segment clock display.

```cpp
Device my_device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
my_device.add_sensor(new Seven_Seg_Clock("001", "Clock"));
```

**Features:**
- 4-digit time display (HH:MM)
- NTP time synchronization
- Rainbow color palette
- Configurable initial brightness
- 60 LEDs (15 per segment)

---

### Thermostats

#### IotCloud_thermostat
Smart thermostat with DS18B20 temperature sensor and relay control.

```cpp
Device device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
iotcloud_setup(&device, OFFLINE_RESTART_TIMEOUT_MS);
```

**Features:**
- DS18B20 temperature sensing
- Hysteresis-based heating control
- Setpoint adjustment via MQTT
- Relay state indication
- NodeMCU-32S (ESP32) target
- Model and semantic firmware version configured in `platformio.ini`
- Model-wide OTA firmware deployment target
- Automatic restart after 15 minutes without IoTCloud connectivity

#### IotCloud_madrid_thermostat

PlatformIO project for the Madrid heating relay on NodeMCU ESP8266. It uses
the `MADRID_SUBSENSOR_THERMOSTAT` model, the `Thermostat` MQTT sensor ID suffix, and the
active-high relay on D2 (GPIO4). The separate sensor publishes temperature and
humidity to the thermostat's MQTT topics. The relay device restarts after 15
minutes without IoTCloud connectivity.

#### IotCloud_sonoff_dual_r3_lite_thermostat
Same thermostat on a modified Sonoff Dual R3 Lite: relay 1 removed, relay 2
(GPIO14) used as a dry contact (OUT1/OUT2) for the heating call, DS18B20 on
GPIO17. The relay is active-high. See its README for wiring.

---

### Specialty Devices

#### IotCloud_washing_machine
Smart washing machine completion detector and remote start button.

```cpp
Device my_device = Device("v1.0", "WASHING_MACHINE");
my_device.add_sensor(new SERVO_BUTTON("001_W", "Washing Machine", ...));
my_device.add_sensor(notifier);
```

**Features:**
- LDR-based LED blinking detection
- Servo-actuated start button
- Push notifications on cycle completion
- Blink pattern analysis for state detection
- Configurable thresholds

#### IotCloud-despacho
Multi-sensor office environment monitor.

**Sensors included:**
- HexaLed lighting
- Presence detection
- CO2 monitoring
- Temperature and humidity

**Features:**
- Complete office automation bundle
- Coordinated sensor operation
- Single device, multiple capabilities

---

## Getting Started

### Prerequisites

1. **Arduino IDE** or **PlatformIO**
2. **IotCloud Library** - Core MQTT and sensor framework
3. **IotCloud Sensors Library** - Sensor base classes
4. **Platform-specific libraries** (ESP8266WiFi, ESP32 Arduino, etc.)

### Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/your-org/IotCloud-devices.git
   ```

2. Install the IotCloud libraries:
   ```bash
   # In Arduino libraries folder
   git clone https://github.com/your-org/IotCloud-arduino-lib.git
   git clone https://github.com/your-org/IotCloud-sensors.git
   ```

3. Open the desired project folder in Arduino IDE or PlatformIO

4. Configure WiFi and MQTT credentials in the device's configuration

5. Upload to your device

### OTA Releases

Projects that extend `ota_template` get an `upload_remote` target
(`pio run -t upload_remote`), which uploads the image and deploys it to every
device of that model. Set `custom_ota_compress = yes` to send a gzip image
(ESP8266 only).

Every build prints an `OTA size:` line with the real image size against the
project's flash layout, and `upload_remote` refuses an image that doesn't fit:

- **ESP32:** the `.bin` must fit one app slot of the project's partition table.
  The "Flash:" line undercounts by about 20 KB (headers and padding).
- **ESP8266:** the update is staged next to the running image, so both must fit
  below the filesystem. The check assumes the device runs a build of about the
  same size; if it runs a smaller one, `custom_ota_size_check = warn` lets the
  upload through.

The check can't see devices of the same model that were flashed with a
different layout (for example a keypad still on default partitions while the
project uses `min_spiffs.csv`). Give those their own `model_num`.

Tests: `python3 -m unittest discover -s tools -p 'test_*.py'`

### Quick Start Example

```cpp
#include <Arduino.h>
#include <iotcloud.h>
#include "my_sensor.h"

Device my_device = Device("v1.0", "MY_DEVICE");

void setup() {
    Serial.begin(115200);
    my_device.add_sensor(new MySensor("001", "SensorName"));
    iotcloud_setup(&my_device);
}

void loop() {
    iotcloud_loop();
}
```

---

## Architecture

### Device Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Arduino Sketch                        │
│  ┌─────────────────────────────────────────────────┐    │
│  │              Device Instance                     │    │
│  │  ┌─────────────┬─────────────┬─────────────┐    │    │
│  │  │   Sensor 1  │   Sensor 2  │   Sensor N  │    │    │
│  │  └─────────────┴─────────────┴─────────────┘    │    │
│  └─────────────────────────────────────────────────┘    │
│                          │                               │
│  ┌───────────────────────┴───────────────────────────┐  │
│  │              IotCloud Library                      │  │
│  │  ┌──────────────┐  ┌──────────────┐              │  │
│  │  │ WiFi Manager │  │ MQTT Client  │              │  │
│  │  └──────────────┘  └──────────────┘              │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
              ┌───────────────────────┐
              │   IotCloud Platform   │
              │   (MQTT Broker)       │
              └───────────────────────┘
```

### Sensor Class Hierarchy

```
Sensor (Base)
├── AnalogSensor          - Numeric values (temperature, humidity, CO2)
├── SwitchSensor          - Binary on/off states
├── Toogle                - Motorized control (up/down/stop)
├── Thermostat            - Temperature control with setpoint
├── LedSensor             - LED strip control with brightness
└── Notifier              - Push notification sender
```

---

## Dependencies

### Core Libraries

| Library | Description |
|---------|-------------|
| `IotCloud-arduino-lib` | Core WiFi, MQTT, and device management |
| `IotCloud-sensors` | Sensor base classes and implementations |
| [`EspMQTTClient`](https://github.com/csanz91/EspMQTTClient) | WiFi/MQTT client, pulled in by `IotCloud-arduino-lib`'s `library.json`. Our fork leaves out ArduinoOTA, mDNS and the web updater unless a device sets `-DESPMQTTCLIENT_ENABLE_OTA` or `-DESPMQTTCLIENT_ENABLE_WEB_UPDATER` |

### Hardware Libraries

| Library | Used By |
|---------|---------|
| `FastLED` | LED controllers |
| `OneWire` | DS18B20 sensors |
| `DallasTemperature` | DS18B20 sensors |
| `Adafruit_SSD1306` | OLED displays |
| `Adafruit_GFX` | Display graphics |
| `Wire` | I2C communication |
| `SoftwareSerial` | MH-Z19 CO2 sensor |

---

## Common Patterns

### Creating a Custom Sensor

```cpp
#pragma once
#include <sensors/analog.h>

class MySensor : public AnalogSensor {
private:
    void get_value() override;  // Called periodically by framework

public:
    MySensor(const char* id, const char* name, const char* metadata);
};
```

```cpp
#include "my_sensor.h"

MySensor::MySensor(const char* id, const char* name, const char* metadata)
    : AnalogSensor(id, name, 30000, 1, true, "analog", metadata, 0.01) {
    // 30000ms interval, CoV=1, retained, 1% filtering
}

void MySensor::get_value() {
    float value = readHardware();
    if (!isnan(value)) {
        set_value(value);  // Publishes to MQTT if changed
    }
}
```

### Metadata JSON Schema

Sensor metadata defines display properties and settings:

```json
{
    "engUnits": "°C",
    "stringFormat": "#0.0",
    "settings": [
        {
            "name": "Offset",
            "description": "Calibration offset",
            "endpoint": "offset",
            "actionType": "floatpar",
            "value": 0.0,
            "limits": {"hi": 10.0, "lo": -10.0}
        }
    ]
}
```

### Switch Debouncing Pattern

```cpp
void loop() {
    bool current_state = digitalRead(SWITCH_PIN);
    if (current_state != _last_state) {
        _debounce_pending = true;
        _debounce_time = millis();
    }
    
    if ((millis() - _debounce_time) > 50) {  // 50ms debounce
        if (_debounce_pending) {
            _debounce_pending = false;
            process_state_change(current_state);
        }
    }
    _last_state = current_state;
}
```

---

## Configuration

### Pin Mappings by Device

#### Sonoff Dual R3
| Function | Constant |
|----------|----------|
| Relay 1  | RELAY_1  |
| Relay 2  | RELAY_2  |
| Switch 1 | SWITCH_1 |
| Switch 2 | SWITCH_2 |

#### NodeMCU/ESP8266
| Function | GPIO | NodeMCU |
|----------|------|---------|
| SI7021   | 5    | D1      |
| CO2 RX   | 13   | D7      |
| CO2 TX   | 12   | D6      |
| LED Data | 1    | TX      |

---

## License

This project is part of the IotCloud ecosystem. See individual project files for specific licensing information.

---

## Contributing

1. Follow the existing sensor class patterns
2. Include proper debouncing for physical inputs
3. Use meaningful sensor IDs and metadata
4. Test on target hardware before submitting
5. Document pin mappings and dependencies
