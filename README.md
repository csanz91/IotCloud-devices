# IotCloud-devices

A comprehensive collection of Arduino/ESP-based IoT device implementations using the IotCloud library ecosystem. This repository contains ready-to-use firmware examples for various sensor and actuator projects that integrate seamlessly with the IotCloud platform.

## Table of Contents

- [Overview](#overview)
- [Repository Structure](#repository-structure)
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
| **ESP8266** | NodeMCU, Sonoff Basic/Mini, Sonoff TH10 |
| **ESP32** | ESP32 DevKit, ESP32-S2, ESP32-C3 |
| **Espruino** | JavaScript-based microcontrollers (via JS implementation) |

---

## Repository Structure

```
IotCloud-devices/
├── IotCloud_7seg_clock/         # 7-segment LED clock display
├── IotCloud_ds18b20/            # DS18B20 temperature sensor
├── IotCloud_chain_waxer/        # Hot pot chain waxer with web portal & Silca setpoints
├── IotCloud_hexaled/            # Hexagonal LED panel controller
├── IotCloud_ld2410/             # LD2410 radar presence sensor
├── IotCloud_ld2410_TH10/        # LD2410 for Sonoff TH10
├── IotCloud_ld2410_esp32/       # LD2410 for ESP32
├── IotCloud_ld2410_light/       # LD2410 with light control
├── IotCloud_letters_lamp/       # RGB letter lamp controller
├── IotCloud_MHZ19/              # MH-Z19 CO2 sensor
├── IotCloud_si7021/             # SI7021 temperature/humidity sensor
├── IotCloud_si7021_MHZ19/       # Combined SI7021 + MH-Z19
├── IotCloud_si7021_oled/        # SI7021 with OLED display
├── IotCloud_sonoff_dual_r3/     # Sonoff Dual R3 (blinds control)
├── IotCloud_sonoff_dual_r3_madrid/  # Sonoff Dual R3 variant (2 switches, presence, BLE scanner)
├── IotCloud_sonoff_dual_r3_lite_ble/  # Sonoff Dual R3 Lite: BLE presence scanner + 2 switches
├── IotCloud_sonoff_mini/        # Sonoff Mini smart switch
├── IotCloud_sonoff_plug/        # Sonoff S26 smart plug
├── IotCloud_sonoff_rf/          # Sonoff RF controller
├── IotCloud_thermostat/         # DS18B20-based thermostat
├── IotCloud_thermostat_subsensor/  # Thermostat subsensor
├── IotCloud_washing_machine/    # Washing machine monitor
├── IotCloud_rotary/             # Rotary encoder device
├── IotCloud-despacho/           # Multi-sensor office device
├── shared/libs/IotCloudBleScanner/  # BLE presence scanner sensor, shared by devices
├── shared/no_ota_stubs/        # No-op ArduinoOTA/mDNS, frees flash (opt-in)
├── iotcloud_t_subsensor_V2.js   # Espruino subsensor implementation
└── README.md
```

---

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

#### IotCloud_MHZ19
CO2 concentration monitoring using the MH-Z19 infrared sensor.

```cpp
Device my_device = Device("v1.3", "CO2");
my_device.add_sensor(new IotCloud_CO2("001_CO2", "CO2", metadata, MH_Z19_RX, MH_Z19_TX));
```

**Features:**
- UART communication with MH-Z19 sensor
- Calibration action via MQTT command
- PPM readings with configurable format

#### IotCloud_ds18b20
Waterproof temperature sensor for outdoor/harsh environments.

```cpp
Device my_device = Device("v1.0", "DS18B20");
my_device.add_sensor(new IOTCLOUD_DS18B20("001_T", "Temperature", ONE_WIRE_BUS));
```

**Features:**
- OneWire protocol support
- 12-bit resolution
- Suitable for pipe/tank monitoring

#### IotCloud_chain_waxer
Bicycle chain hot-pot waxing station with DS18B20 digital temperature sensor, real-time web portal (`http://chainwaxer.local`), and preset waxing stages.

```cpp
Device device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
ChainWaxerSensor waxer_sensor("001_T", "HotPot Wax Temp", ONE_WIRE_BUS);
WaxerWebPortal web_portal(waxer_sensor);
```

**Features:**
- Real-time standalone web portal (`http://chainwaxer.local`) with live temperature readout, chart, and audio chime
- Temperature setpoint presets:
  - **125°C**: Initial heating & factory grease removal
  - **85°C**: Regular waxing soak
  - **72°C**: Chain removal & pull temp (optimal coating thickness)
- Automatic 10-minute immersion/soak timer with progress bar
- Non-blocking 1-Wire DS18B20 sampling on D1 (GPIO5), D2, D5, or D6
- Full IotCloud MQTT integration with offset calibration and captive portal fallback


#### IotCloud_si7021_MHZ19
Combined air quality monitoring station.

**Features:**
- Temperature, humidity, and CO2 monitoring
- Configurable offset settings for calibration
- All-in-one indoor air quality solution

#### IotCloud_si7021_oled
Temperature/humidity sensor with OLED information display.

**Features:**
- 128x32 OLED display support
- Scrolling message display from external server
- Real-time sensor value visualization

---

### Smart Switches & Relays

#### IotCloud_sonoff_mini
Compact smart switch for in-wall installation.

```cpp
Device my_device = Device("v1.2.5", "SONOFF_MINI");
my_device.add_sensor(new GENERIC_TOOGLE("SonoffMini", "Switch", RELAY_PIN, SWITCH_PIN));
```

**Features:**
- Toggle mode operation
- Physical switch debouncing (50ms)
- MQTT state synchronization
- GPIO 12 relay, GPIO 4 switch input

#### IotCloud_sonoff_dual_r3
Dual-relay controller for motorized blinds/shades.

```cpp
Device my_device = Device("v1.0.4", "SONOFF_DUAL_R3");
my_device.add_sensor(new GENERIC_TOOGLE("SonoffDualR3", "Persiana", RELAY_1, RELAY_2, SWITCH_1, SWITCH_2));
```

**Features:**
- Bidirectional motor control (UP/DOWN)
- Configurable action duration (1-30 seconds)
- Dual switch input support
- Auto-stop on opposite switch activation
- MQTT settings for movement duration

#### IotCloud_sonoff_dual_r3_lite_ble
Sonoff Dual R3 Lite at the house entry: listens for the phones' BLE presence
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
`IotCloud_8button_keypad` (`min_spiffs`). This device runs the same libraries
as Madrid, so test new scanner firmware here before sending it to Madrid.

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

#### IotCloud_hexaled
Hexagonal RGBW LED panel controller with rainbow effects.

```cpp
Device my_device = Device("v1.0", "HEXALED");
my_device.add_sensor(new HexaLed("001", "Hexaled"));
```

**Features:**
- WS2812/SK6812 RGBW support via FastLED
- 8 hexagon configuration (160 LEDs)
- Rainbow gradient effect
- Brightness control via MQTT

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
Device my_device = Device("v1.4", "7SEG_CLOCK");
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

#### Sonoff Mini
| Function | GPIO | NodeMCU |
|----------|------|---------|
| Relay    | 12   | -       |
| Switch   | 4    | D2      |
| LED      | 13   | -       |

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
