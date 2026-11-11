#include <Arduino.h>
#include <iotcloud.h>
#include <ble_scanner_sensor.h>
#include <ota_confirm.h>
#include "keypad_sensor.h"

namespace {

constexpr uint8_t BUTTON_COUNT = 8;
// ESP32-C3 keypad inputs. Avoid GPIO8/GPIO9 boot straps, GPIO18/GPIO19 USB,
// GPIO20/GPIO21 UART, and GPIO11-GPIO17 flash pins.
constexpr uint8_t BUTTON_PINS[BUTTON_COUNT] = {7, 6, 5, 4, 0, 1, 2, 3};
constexpr unsigned long DEBOUNCE_MS = 40;
constexpr unsigned long LONG_PRESS_MS = 1000;
constexpr unsigned long STARTUP_GRACE_MS = 3000;

bool stable_states[BUTTON_COUNT] = {false};
bool last_readings[BUTTON_COUNT] = {false};
unsigned long last_change_ms[BUTTON_COUNT] = {0};
unsigned long pressed_at_ms[BUTTON_COUNT] = {0};
bool long_press_sent[BUTTON_COUNT] = {false};

Device my_device = Device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
KeypadSensor *keypad_sensor = new KeypadSensor("001_E", "Bedroom Keypad");
BleScannerSensor ble_scanner("001_BLE", "BLE presence");

constexpr uint32_t OTA_CONFIRM_AFTER_MS = 60000;

void handle_buttons() {
  const unsigned long now = millis();
  const bool startup_grace_active = now < STARTUP_GRACE_MS;

  for (uint8_t index = 0; index < BUTTON_COUNT; index++) {
    const bool reading = digitalRead(BUTTON_PINS[index]) == HIGH;

    if (reading != last_readings[index]) {
      last_readings[index] = reading;
      last_change_ms[index] = now;
    }

    if (now - last_change_ms[index] < DEBOUNCE_MS) {
      continue;
    }

    if (reading == stable_states[index]) {
      if (stable_states[index] && !long_press_sent[index] &&
          now - pressed_at_ms[index] >= LONG_PRESS_MS) {
        long_press_sent[index] = true;
        if (!startup_grace_active) {
          keypad_sensor->publish_long_key(index + 1);
        }
      }
      continue;
    }

    stable_states[index] = reading;

    if (stable_states[index]) {
      pressed_at_ms[index] = now;
      long_press_sent[index] = false;
    } else if (!long_press_sent[index]) {
      if (!startup_grace_active) {
        keypad_sensor->publish_key(index + 1);
      }
    }
  }
}

}  // namespace

// Keep a new OTA image pending until the scanner and MQTT have run for a while.
extern "C" bool verifyRollbackLater() { return true; }

void setup(void) {
  for (uint8_t index = 0; index < BUTTON_COUNT; index++) {
    pinMode(BUTTON_PINS[index], INPUT_PULLDOWN);
    const bool reading = digitalRead(BUTTON_PINS[index]) == HIGH;
    stable_states[index] = reading;
    last_readings[index] = reading;
  }

  my_device.add_sensor(keypad_sensor);
  my_device.add_sensor(&ble_scanner);

  // Scan only once provisioned; setup/AP mode doesn't need the radio shared.
  const device_data saved_data = get_data();
  if (validate_device_data(&saved_data)) {
    ble_scanner.begin_scanning();
  }

  iotcloud_setup(&my_device);

}

void loop(void) {
  iotcloud_loop();
  handle_buttons();
  ota_confirm_when(ble_scanner.healthy_for(OTA_CONFIRM_AFTER_MS));
}
