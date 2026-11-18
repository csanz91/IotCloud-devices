#pragma once

#if defined(ARDUINO) && defined(ESP32)

#include <Arduino.h>
#include <atomic>
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
//
// Talks HCI to the BT controller directly (VHCI) instead of using a BLE host
// stack: a scan-only device doesn't need one, and it saves flash.
class BleScannerSensor : public BaseSensor
{
public:
    BleScannerSensor(const char *sensor_id, const char *sensor_name);

    // Starts the BT controller and the continuous scan. Call once, only when
    // the device is provisioned.
    void begin_scanning();
    void loop() override;

    // True once the scan is running and MQTT has stayed connected for
    // `min_ms`. Pass it to OTA::set_health_check so a new OTA image is only
    // confirmed (and rollback cancelled) once the scanner works.
    bool healthy_for(uint32_t min_ms) const;

    // Called from the controller task. Must stay short.
    void on_controller_packet(const uint8_t *data, uint16_t len);
    void enqueue_sample(const uint8_t token[BLE_TOKEN_LEN], int8_t rssi);

private:
    struct Sample
    {
        uint8_t token[BLE_TOKEN_LEN];
        int8_t rssi;
    };

    enum class State : uint8_t
    {
        Off,      // begin_scanning() not called or controller failed
        Starting, // sending the init commands one at a time
        Scanning,
        Backoff, // init failed, retry later
    };

    static constexpr size_t QUEUE_LENGTH = 64;
    static constexpr uint8_t INIT_STEPS = 4;
    static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;
    static constexpr uint32_t RETRY_DELAY_MS = 5000;
    // A house is never this quiet: no advertising at all means scanning stopped.
    static constexpr uint32_t SILENCE_RESTART_MS = 60000;
    static constexpr uint32_t STATS_LOG_INTERVAL_MS = 60000;

    QueueHandle_t _queue = nullptr;
    BleSightingTracker _tracker;
    State _state = State::Off;
    uint8_t _step = 0;
    bool _step_sent = false;
    uint32_t _step_sent_ms = 0;
    uint32_t _backoff_since_ms = 0;
    uint32_t _scanning_since_ms = 0;
    uint32_t _mqtt_connected_since_ms = 0;
    bool _mqtt_was_connected = false;
    uint32_t _last_stats_log_ms = 0;
    uint32_t _table_drops = 0;

    // Written by the controller task.
    std::atomic<uint32_t> _command_result{0}; // 1 << 24 | opcode << 8 | status
    std::atomic<uint32_t> _last_adv_ms{0};
    std::atomic<uint32_t> _queue_drops{0};

    void run_init(uint32_t now_ms);
    void restart_init(uint32_t now_ms, const __FlashStringHelper *reason);
    bool send_step();
    void track_mqtt(uint32_t now_ms);
    void publish(const BleSightingReport &report);
    void log_stats(uint32_t now_ms);
};

#endif // ARDUINO && ESP32
