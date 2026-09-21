# Code Review: Issues & Code Smells

A comprehensive review of the IotCloud-devices repository identifying bugs, code smells, and potential issues.

---

## Table of Contents

- [Critical Issues](#critical-issues)
- [Bugs](#bugs)
- [Code Smells](#code-smells)
- [Naming Inconsistencies](#naming-inconsistencies)
- [Memory & Resource Issues](#memory--resource-issues)
- [Security Concerns](#security-concerns)
- [Code Duplication](#code-duplication)
- [Technical Debt](#technical-debt)

---

## Critical Issues

### 1. NULL Comparison with Float (ds18b20_sensor.cpp:22)

```cpp
if (value != NULL)  // BUG: Comparing float to NULL
```

**File:** `IotCloud_ds18b20/ds18b20_sensor.cpp`

**Problem:** Comparing a `float` to `NULL` is incorrect. NULL is typically `0` or `nullptr`, and this comparison may not work as intended.

**Fix:**
```cpp
if (!isnan(value) && value != DEVICE_DISCONNECTED_C)
```

---

### 2. Missing State Inversion Logic (sonoff_mini - generic_switch.cpp:44)

```cpp
GENERIC_TOOGLE::set_state(!state);  // Uses class member 'state'
```

**File:** `IotCloud_sonoff_mini/generic_switch.cpp`

**Problem:** The code uses `!state` where `state` is a member variable from the parent class. This toggles based on the current MQTT state, not the physical switch state. Combined with the debounce logic, this may cause unexpected behavior.

**Expected:** Toggle should be based on physical switch state change, not just invert current state.

---

### 3. JavaScript Memory Leak via eval() (iotcloud_t_subsensor_V2.js:388-392)

```javascript
eval(storage.read('runtime'));  // Dangerous eval usage
eval(storage.read('setup'));
```

**File:** `iotcloud_t_subsensor_V2.js`

**Problem:** Using `eval()` to execute code stored in flash is dangerous and can lead to memory leaks, security issues, and debugging difficulties.

---

### 4. Missing Break Statement (ds18b20_thermostat.cpp:50-54)

```cpp
case Steps::GET_ADDR:
{
    // ...
    if (found)
    {
        _current_step = Steps::REQ_TEMP;
    }
    else
    {
        break;  // Only breaks on NOT found
    }
}
case Steps::REQ_TEMP:  // Falls through intentionally but unclear
```

**File:** `IotCloud_thermostat/ds18b20_thermostat.cpp`

**Problem:** Intentional fall-through from `GET_ADDR` to `REQ_TEMP` is not documented. This is error-prone and confusing.

**Fix:** Add explicit `[[fallthrough]]` annotation or restructure the state machine.

---

## Bugs

### 5. Incomplete Debounce in Dual R3 (generic_toogle.cpp:148-152)

```cpp
// else if (switch_state)
// {
//     _mem_switch_deact_state_2 = false;
// }
```

**File:** `IotCloud_sonoff_dual_r3/generic_toogle.cpp`

**Problem:** Commented-out code for switch 2 deactivation handling that exists for switch 1. This asymmetry may cause intermittent bugs where switch 2 doesn't debounce correctly.

---

### 6. Potential Integer Overflow in Timing (multiple files)

```cpp
unsigned long ellapsed_since_last_read = millis() - last_reading;
```

**Files:** `IotCloud_si7021/IotCloud_si7021.ino`, `IotCloud_washing_machine/IotCloud_washing_machine.ino`, etc.

**Problem:** If `millis()` overflows (every ~49 days), and `last_reading` was set before overflow, the subtraction will still work correctly due to unsigned arithmetic. However, this should be documented for maintenance clarity.

---

### 7. Hardcoded Server IP (si7021_oled - IotCloud_si7021_oled.ino:30-31)

```cpp
const char *serverIP = "192.168.0.155";
const int serverPort = 8000;
```

**File:** `IotCloud_si7021_oled/IotCloud_si7021_oled.ino`

**Problem:** Hardcoded IP address won't work after network changes or on different deployments.

---

### 8. Infinite Loop on Display Failure (si7021_oled - IotCloud_si7021_oled.ino:74-75)

```cpp
if (!display.begin(...))
{
    for (;;) ;  // Don't proceed, loop forever
}
```

**Problem:** Device becomes completely unresponsive if display fails. Should fallback to headless operation.

---

### 9. Unchecked Pointer Dereference (JavaScript - iotcloud_t_subsensor_V2.js:336-338)

```javascript
const sensor = device.sensors.find(...);
sensor.sendCommand(command);  // No null check!
```

**Problem:** If sensor is not found, this will throw an error.

---

## Code Smells

### 10. Inconsistent Class Naming Convention

| File | Class Name | Issue |
|------|------------|-------|
| `IotCloud_ld2410/generic_switch.h` | `GENERIC_TOOGLE` | ALL_CAPS naming |
| `IotCloud_sonoff_dual_r3/generic_toogle.h` | `GENERIC_TOOGLE` | ALL_CAPS + typo |
| `IotCloud_thermostat/ds18b20_thermostat.h` | `IOTCLOUD_THERMOSTAT_DS18B20` | ALL_CAPS |
| `IotCloud_7seg_clock/seven_seg_clock.h` | `Seven_Seg_Clock` | Mixed_Case |
| `IotCloud_hexaled/hexa_led.h` | `HexaLed` | PascalCase ✓ |
| `IotCloud_letters_lamp/rgb_lamp.h` | `RgbLamp` | PascalCase ✓ |

**Recommendation:** Standardize on PascalCase for class names.

---

### 11. Typo in Class Name: "TOOGLE" → "TOGGLE"

**Files:** 
- `IotCloud_sonoff_dual_r3/generic_toogle.h`
- `IotCloud_sonoff_dual_r3/generic_toogle.cpp`
- `IotCloud_sonoff_mini/generic_switch.h` (class name)
- `IotCloud_ld2410/generic_switch.h` (class name)

**Problem:** "Toggle" is misspelled as "Toogle" consistently across multiple files.

---

### 12. Inconsistent Header Guard Styles

```cpp
// Style 1: pragma once
#pragma once

// Style 2: Traditional guards
#ifndef _HEXA_LED
#define _HEXA_LED
...
#endif // _HEXA_LED
```

**Files:** Mixed usage across the codebase.

**Recommendation:** Standardize on `#pragma once` for simplicity.

---

### 13. Magic Numbers Without Constants

```cpp
// IotCloud_si7021/temp_hum_sensor.cpp
AnalogSensor(sensor_id, sensor_name, 30000, 1, true, "analog", metadata, 0.01)
//                                   ^^^^^  ^       ^^^^

// IotCloud_ld2410/generic_switch.cpp
if ((millis() - _last_debounce_time_1) > 50)  // Magic 50ms
```

**Problem:** Numeric literals scattered throughout code without named constants.

**Fix:**
```cpp
static constexpr unsigned int DEBOUNCE_MS = 50;
static constexpr unsigned long POLL_INTERVAL_MS = 30000;
```

---

### 14. Large Inline JSON Strings

```cpp
my_device.add_sensor(new IotCloud_CO2("001_CO2", "CO2", 
    "{\"engUnits\":\"ppm\",\"stringFormat\":\"#0\",\"settings\":[{\"name\":\"Calibrate\",\"description\":\"Place the sensor in an eviroment with fresh air.\",\"endpoint\":\"calibrate\",\"actionType\":\"action\",\"value\":true}]}",
    MH_Z19_RX, MH_Z19_TX));
```

**Files:** Multiple `.ino` files.

**Problem:** 
- Hard to read and maintain
- Typos in JSON go unnoticed (e.g., "eviroment" instead of "environment")
- Difficult to validate JSON structure

**Fix:** Use separate JSON files or PROGMEM constants.

---

### 15. Duplicate Code Across Projects

**Identical files found:**
- `temp_hum_sensor.cpp/h` in:
  - `IotCloud_si7021/`
  - `IotCloud_si7021_MHZ19/`
  - `IotCloud_si7021_oled/`
  - `IotCloud_washing_machine/`
  - `IotCloud-despacho/`

**Problem:** Changes must be replicated across 5+ copies.

**Fix:** Move to shared library or use PlatformIO lib_deps.

---

### 16. Commented-Out Code Left in Production

```cpp
// IotCloud_hexaled/hexa_led.cpp:22-62
// static uint16_t sPseudotime = 0;
// static uint16_t sLastMillis = 0;
// ...
// (40+ lines of commented code)
```

**Problem:** Dead code clutters the codebase and creates confusion.

---

### 17. Long Method: loop() in Washing Machine (126 lines main file)

**File:** `IotCloud_washing_machine/IotCloud_washing_machine.ino`

**Problem:** Complex state machine logic mixed with sensor reading in a single file without proper encapsulation.

---

## Memory & Resource Issues

### 18. Static Memory Allocation for Variable LED Count

```cpp
// IotCloud_letters_lamp/rgb_lamp.h
static constexpr int NUM_MAX_LEDS = 255;
CRGB _leds[NUM_MAX_LEDS];  // Always allocates 255 * 3 = 765 bytes
```

**Problem:** Even if using only 10 LEDs, 765 bytes are allocated.

---

### 19. Dynamic Memory Allocation in Constructor

```cpp
// IotCloud_thermostat/ds18b20_thermostat.cpp
IOTCLOUD_THERMOSTAT_DS18B20::IOTCLOUD_THERMOSTAT_DS18B20(...) {
    _oneWire = new OneWire(one_wire_bus);
    _sensors = new DallasTemperature(_oneWire);
}
```

**Problem:** No corresponding destructor to free memory. Memory leak if object is ever deleted.

---

### 20. Global Variables Instead of Class Members

```cpp
// IotCloud_si7021/IotCloud_si7021.ino
unsigned long last_reading = 0;  // Global variable
```

**Problem:** Pollutes global namespace and makes testing difficult.

---

## Security Concerns

### 21. Hardcoded WiFi Hotspot Password

```javascript
// iotcloud_t_subsensor_V2.js:177-178
const defaultApSSID = 'iot_'+deviceInternalId.split('-')[1];
const defaultApPassword = "12345678";
```

**Problem:** Weak default password that users may not change.

---

### 22. Unvalidated MQTT Message Parsing

```javascript
// iotcloud_t_subsensor_V2.js:317-326
version = subtopics[0];
locationId = subtopics[1];
deviceId = subtopics[2];
sensorId = subtopics[3];
endpoint = subtopics[4];
```

**Problem:** No validation of array bounds before accessing elements.

---

## Code Duplication

### 23. Repeated Debouncing Pattern

The debounce pattern is reimplemented in multiple files:
- `IotCloud_sonoff_dual_r3/generic_toogle.cpp`
- `IotCloud_sonoff_mini/generic_switch.cpp`
- `IotCloud_ld2410/generic_switch.cpp`

**Fix:** Create a reusable `Debouncer` utility class.

---

### 24. Repeated Switch Logic in Dual R3

```cpp
// Switch 1 handling (lines 64-108)
bool switch_state = !digitalRead(_switch1_pin);
if (switch_state && !_last_state_1) { ... }
// ... 44 lines

// Switch 2 handling (lines 110-154) - Nearly identical
switch_state = !digitalRead(_switch2_pin);
if (switch_state && !_last_state_2) { ... }
```

**Problem:** Copy-paste code with only variable names changed.

**Fix:** Extract to a method or use arrays for switch handling.

---

## Technical Debt

### 25. Missing Error Handling

```cpp
// IotCloud_despacho/light_sensor.cpp:20
lightMeter.begin();  // Return value ignored
```

Many hardware initialization calls don't check return values.

---

### 26. No Unit Tests

The repository contains no test files. Critical logic like:
- Debouncing
- State machine transitions
- Value filtering

Cannot be verified without hardware.

---

### 27. Inconsistent File Extensions

- Some README files have extensions: `README.md`
- Others don't: `README` (in `IotCloud_sonoff_dual_r3/`, `IotCloud_ld2410/`, etc.)

---

### 28. Missing `.gitignore` Patterns

The `.gitignore` in `IotCloud_hexaled/` contains:
```
.pio
.vscode/.browse.c_cpp.db*
.vscode/c_cpp_properties.json
.vscode/launch.json
.vscode/ipch
```

But many projects have `.vscode/` folders checked in with settings files.

---

### 29. Build Artifacts Committed

```
IotCloud-despacho/build/
```

Build directories should not be committed to version control.

---

### 30. Inconsistent Sensor ID Naming

| Project | Sensor ID | Pattern |
|---------|-----------|---------|
| si7021 | `001_T`, `001_H` | numeric_type |
| MHZ19 | `001_CO2` | numeric_type |
| sonoff_mini | `SonoffMini` | DeviceName |
| ld2410 | `BathroomLight` | LocationFunction |
| thermostat | `Thermostat` | Type |

**Problem:** No consistent naming convention for sensor IDs.

---

## Summary

| Category | Count | Severity |
|----------|-------|----------|
| Critical Issues | 4 | 🔴 High |
| Bugs | 5 | 🟠 Medium |
| Code Smells | 8 | 🟡 Low |
| Memory Issues | 3 | 🟠 Medium |
| Security Concerns | 2 | 🟠 Medium |
| Code Duplication | 2 | 🟡 Low |
| Technical Debt | 6 | 🟡 Low |

**Total Issues: 30**

---

## Recommended Priority

1. **Immediate:** Fix NULL comparison bug (#1), missing break statement (#4)
2. **Short-term:** Address security concerns, fix typos in class names
3. **Medium-term:** Consolidate duplicate code into shared library
4. **Long-term:** Add unit tests, standardize naming conventions
