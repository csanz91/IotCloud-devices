# IotCloud Chain Waxer (NodeMCU ESP8266 + DS18B20)

A precision hot-pot temperature controller and monitoring station for bicycle chain hot-melt waxing. Designed with optimal chain preparation and waxing temperatures, featuring a standalone real-time web portal and full integration with the **IotCloud** ecosystem.

---

## Features

- **Real-Time Web Portal (`http://chainwaxer.local`)**:
  - Live temperature readout updating every second.
  - Quick-select buttons for standard **waxing stages**.
  - Dynamic status banner and live guidance instructions.
  - Automatic immersion/soak timer with 10-minute target progress bar.
  - Embedded Canvas temperature chart (runs 100% offline, no internet or CDN needed).
  - Audio alert chime (HTML5 Web Audio API) when target temperature is reached or chain is ready to pull.
  - Min/Max session temperature tracking and offset calibration.
- **IotCloud Ecosystem Integration**:
  - Automatically provisions WiFi and MQTT via the IotCloud captive portal if unconfigured.
  - Publishes real-time temperature to the cloud/MQTT broker (`<header>/001_T/value`).
  - Supports dynamic temperature offset calibration via MQTT topic (`.../aux/offset`).
  - Over-The-Air (OTA) firmware upgrade support.
- **High Performance & Non-blocking**:
  - 1-Wire DS18B20 conversion runs completely asynchronously without `delay()`, ensuring the web portal and IoTCloud loop remain fast and responsive.

---

## Hardware Wiring

| NodeMCU ESP8266 Pin | DS18B20 Wire | Description |
|---------------------|--------------|-------------|
| **3V3** (3.3V)      | Red (VCC)    | Power (3.3V) |
| **GND**             | Black (GND)  | Ground |
| **D1** (GPIO5)      | Yellow (DATA)| 1-Wire Data Line (*D2, D5, and D6 also supported*) |

> [!IMPORTANT]
> **Pull-Up Resistor:** A **4.7 kΩ** resistor must be connected between the **3.3V** pin and the **DATA (D1)** line. Without this pull-up resistor, 1-Wire communication will fail.

### Using D2, D5, or D6 Instead of D1
If you prefer using **D2 (GPIO4)**, **D5 (GPIO14)**, or **D6 (GPIO12)**:
- In `platformio.ini`, add build flag: `-D ONE_WIRE_BUS=D2` (or `D5`, `D6`).
- Or edit `#define ONE_WIRE_BUS D2` at the top of `src/IotCloud_chain_waxer.cpp`.
The web portal and serial output automatically detect and display whichever pin is compiled.

---

## Temperature Setpoints & Stages

| Stage | Target Temp | Description & Process |
|-------|:-----------:|-----------------------|
| 🧪 **Strip Grease** | **125°C** (257°F) | **Factory Grease Stripping:** High-temperature breakdown cycle to strip factory grease on new chains directly in wax. Submerge new chain, agitate, and soak for 10 min. *(DS18B20 max rated limit)*. |
| 🚴 **Regular Wax** | **85°C** (185°F) | **Hot Melt Wax Soak:** Optimal melt immersion range (75°C–88°C). Allows hot wax to deeply penetrate between pins, plates, and inner rollers. Submerge chain, swish thoroughly, and soak for 5–10 min. |
| ⛓️ **Chain Removal** | **72°C** (162°F) | **Optimal Pull/Hang Temp:** Recommended pull temperature as bath cools (70°C–75°C). At this temperature, the wax thickens slightly, ensuring maximum wax retention inside rollers rather than running off. |
| ⚙️ **Custom Target** | *40°C – 130°C* | User-configurable setpoint for any custom wax blend. |

---

## Web Portal Usage

1. Connect the NodeMCU to USB power.
2. Ensure the device is connected to your WiFi (or complete the initial IotCloud setup captive portal).
3. On any smartphone, tablet, or PC on the same WiFi network, open:
   ```text
   http://chainwaxer.local
   ```
   *(Or navigate directly to the ESP8266 IP address shown on serial monitor).*
4. Select your desired stage:
   - Click **Strip Grease (125°)** if preparing a new chain.
   - Click **Regular Wax (85°)** for routine hot-waxing.
   - When finished soaking, turn off/down heat and click **Pull Chain (72°)** to be alerted when it's time to hang the chain.

---

## REST API Endpoints

The web portal exposes JSON REST endpoints:

- `GET /api/status`: Returns full status JSON (temperature, target, stage, timer, min/max, RSSI, address).
- `POST /api/stage?stage=strip|wax|remove|standby`: Changes active waxing stage.
- `POST /api/target?temp=85.0`: Sets a custom target temperature.
- `POST /api/timer/reset`: Resets the soak timer.
- `POST /api/timer/toggle`: Pauses or resumes the soak timer.
- `POST /api/offset?val=0.5`: Adjusts calibration temperature offset.

---

## Compiling & Flashing

```bash
# Build firmware
~/.platformio/penv/bin/pio run -d IotCloud_chain_waxer

# Flash via USB
~/.platformio/penv/bin/pio run -d IotCloud_chain_waxer --target upload

# Monitor Serial
~/.platformio/penv/bin/pio run -d IotCloud_chain_waxer --target monitor
```
