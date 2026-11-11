#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <sensors/sensor.h>

#include "ble_beacon.h"

// Passive scan timing. BLE and Wi-Fi share one radio: a 50% duty cycle keeps
// MQTT responsive. Raise the window (up to the interval) to hear more packets
// if Wi-Fi stays stable; lower it if MQTT starts dropping.
#ifndef BLE_SCAN_INTERVAL_MS
#define BLE_SCAN_INTERVAL_MS 100
#endif
#ifndef BLE_SCAN_WINDOW_MS
#define BLE_SCAN_WINDOW_MS 50
#endif

// Sensor of type `ble_scanner`. Listens for phone presence beacons and
// forwards per-token RSSI means and `gone` events to
// `{header}{sensor_id}/aux/ble/sighting`. Holds no secrets.
class BleScannerSensor : public BaseSensor
{
public:
    BleScannerSensor(const char *sensor_id, const char *sensor_name);

    // Starts NimBLE and the continuous scan. Call once, only in runtime mode.
    void begin_scanning();
    void loop() override;

    // Called from the NimBLE host task.
    void enqueue_sample(const uint8_t token[BLE_TOKEN_LEN], int8_t rssi);

private:
    struct Sample
    {
        uint8_t token[BLE_TOKEN_LEN];
        int8_t rssi;
    };

    static constexpr size_t QUEUE_LENGTH = 64;
    static constexpr uint32_t SCAN_CHECK_INTERVAL_MS = 5000;
    static constexpr uint32_t DROP_LOG_INTERVAL_MS = 60000;

    QueueHandle_t _queue = nullptr;
    BleSightingTracker _tracker;
    bool _scanning = false;
    uint32_t _last_scan_check_ms = 0;
    uint32_t _last_drop_log_ms = 0;
    volatile uint32_t _queue_drops = 0;
    uint32_t _table_drops = 0;

    void ensure_scanning(uint32_t now_ms);
    void publish(const BleSightingReport &report);
    void log_drops(uint32_t now_ms);
};
