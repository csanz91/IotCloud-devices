#if defined(ARDUINO) && defined(ESP32)

#include "ble_scanner_sensor.h"

#include <esp32-hal-bt.h>
#include <esp_bt.h>
// Tells the core this firmware uses BLE, so it doesn't free the BLE memory at boot.
#include <esp32-hal-alloc-ble-mem.h>

#include "ble_hci.h"

namespace
{
constexpr char SIGHTING_ENDPOINT[] = "aux/ble/sighting";
constexpr uint32_t COMMAND_DONE_FLAG = 1u << 24;

// VHCI has a single host; this is the sensor that owns it.
BleScannerSensor *vhci_owner = nullptr;

void on_send_available() {}

int on_packet(uint8_t *data, uint16_t len)
{
    if (vhci_owner != nullptr)
        vhci_owner->on_controller_packet(data, len);
    return 0;
}

esp_vhci_host_callback_t vhci_callbacks = {on_send_available, on_packet};

void beacon_sink(void *ctx, const uint8_t token[BLE_TOKEN_LEN], int8_t rssi)
{
    static_cast<BleScannerSensor *>(ctx)->enqueue_sample(token, rssi);
}
} // namespace

BleScannerSensor::BleScannerSensor(const char *sensor_id, const char *sensor_name)
    : BaseSensor(sensor_id, sensor_name, "ble_scanner", "{}")
{
}

void BleScannerSensor::begin_scanning()
{
    if (_state != State::Off || vhci_owner != nullptr)
        return;

    _queue = xQueueCreate(QUEUE_LENGTH, sizeof(Sample));
    if (_queue == nullptr)
    {
        IOTCLOUD_LOG_PRINTLN(F("BLE: queue allocation failed"));
        return;
    }

    if (!btStartMode(BT_MODE_BLE))
    {
        IOTCLOUD_LOG_PRINTLN(F("BLE: controller start failed"));
        return;
    }

    vhci_owner = this;
    if (esp_vhci_host_register_callback(&vhci_callbacks) != ESP_OK)
    {
        IOTCLOUD_LOG_PRINTLN(F("BLE: VHCI register failed"));
        vhci_owner = nullptr;
        return;
    }

    const uint32_t now = millis();
    _last_stats_log_ms = now;
    _state = State::Starting;
    _step = 0;
    _step_sent = false;
}

void BleScannerSensor::on_controller_packet(const uint8_t *data, uint16_t len)
{
    BleHciEventInfo info;
    ble_hci_parse_event(data, len, info, beacon_sink, this);
    if (info.adv_reports > 0)
        _last_adv_ms.store(millis());
    if (info.command_done)
        _command_result.store(COMMAND_DONE_FLAG | (uint32_t(info.opcode) << 8) | info.status);
}

void BleScannerSensor::enqueue_sample(const uint8_t token[BLE_TOKEN_LEN], int8_t rssi)
{
    Sample sample;
    memcpy(sample.token, token, BLE_TOKEN_LEN);
    sample.rssi = rssi;
    if (xQueueSend(_queue, &sample, 0) != pdTRUE)
        _queue_drops.fetch_add(1);
}

bool BleScannerSensor::send_step()
{
    uint8_t cmd[BLE_HCI_MAX_COMMAND_LEN];
    size_t len = 0;
    switch (_step)
    {
    case 0:
        len = ble_hci_cmd_reset(cmd);
        break;
    case 1:
        len = ble_hci_cmd_set_event_mask(cmd);
        break;
    case 2:
        len = ble_hci_cmd_le_set_scan_params(cmd, BLE_SCAN_INTERVAL_MS, BLE_SCAN_WINDOW_MS);
        break;
    default:
        len = ble_hci_cmd_le_set_scan_enable(cmd, true);
        break;
    }
    if (!esp_vhci_host_check_send_available())
        return false;
    _command_result.store(0);
    esp_vhci_host_send_packet(cmd, len);
    return true;
}

void BleScannerSensor::run_init(uint32_t now_ms)
{
    if (!_step_sent)
    {
        if (send_step())
        {
            _step_sent = true;
            _step_sent_ms = now_ms;
        }
        return;
    }

    static constexpr uint16_t STEP_OPCODES[INIT_STEPS] = {
        BLE_HCI_OP_RESET, BLE_HCI_OP_SET_EVENT_MASK,
        BLE_HCI_OP_LE_SET_SCAN_PARAMS, BLE_HCI_OP_LE_SET_SCAN_ENABLE};

    const uint32_t result = _command_result.load();
    const uint16_t opcode = (result >> 8) & 0xFFFF;
    if ((result & COMMAND_DONE_FLAG) == 0 || opcode != STEP_OPCODES[_step])
    {
        if (now_ms - _step_sent_ms >= COMMAND_TIMEOUT_MS)
            restart_init(now_ms, F("command timeout"));
        return;
    }

    const uint8_t status = result & 0xFF;
    if (status != 0)
    {
        IOTCLOUD_LOG_PRINT(F("BLE: HCI step "));
        IOTCLOUD_LOG_PRINT(_step);
        IOTCLOUD_LOG_PRINT(F(" status 0x"));
        IOTCLOUD_LOG_PRINTLN(String(status, HEX));
        restart_init(now_ms, F("command failed"));
        return;
    }

    _step++;
    _step_sent = false;
    if (_step >= INIT_STEPS)
    {
        _state = State::Scanning;
        _scanning_since_ms = now_ms;
        _last_adv_ms.store(now_ms);
        IOTCLOUD_LOG_PRINTLN(F("BLE: scanning"));
    }
}

void BleScannerSensor::restart_init(uint32_t now_ms, const __FlashStringHelper *reason)
{
    IOTCLOUD_LOG_PRINT(F("BLE: restarting scan, "));
    IOTCLOUD_LOG_PRINTLN(reason);
    _state = State::Backoff;
    _backoff_since_ms = now_ms;
}

void BleScannerSensor::loop()
{
    if (_state == State::Off)
        return;

    const uint32_t now = millis();
    track_mqtt(now);

    switch (_state)
    {
    case State::Starting:
        run_init(now);
        break;
    case State::Backoff:
        if (now - _backoff_since_ms >= RETRY_DELAY_MS)
        {
            _state = State::Starting;
            _step = 0;
            _step_sent = false;
        }
        break;
    case State::Scanning:
        if (now - _last_adv_ms.load() >= SILENCE_RESTART_MS)
            restart_init(now, F("no advertising heard"));
        break;
    default:
        break;
    }

    Sample sample;
    while (xQueueReceive(_queue, &sample, 0) == pdTRUE)
    {
        if (!_tracker.add_sample(sample.token, sample.rssi, now))
            _table_drops++;
    }

    BleSightingReport report;
    while (_tracker.poll(now, report))
        publish(report);

    log_stats(now);
}

void BleScannerSensor::track_mqtt(uint32_t now_ms)
{
    const bool connected = _mqtt_client != nullptr && _mqtt_client->isConnected();
    if (connected && !_mqtt_was_connected)
        _mqtt_connected_since_ms = now_ms;
    _mqtt_was_connected = connected;
}

bool BleScannerSensor::healthy_for(uint32_t min_ms) const
{
    if (_state != State::Scanning || !_mqtt_was_connected)
        return false;
    const uint32_t now = millis();
    return now - _scanning_since_ms >= min_ms && now - _mqtt_connected_since_ms >= min_ms;
}

void BleScannerSensor::publish(const BleSightingReport &report)
{
    // Sightings are only useful live: drop them while offline (QoS 0, not retained).
    if (_mqtt_client == nullptr || !_mqtt_client->isConnected())
        return;

    char payload[64];
    int len;
    if (report.gone)
    {
        len = snprintf(payload, sizeof(payload), "{\"t\":\"%s\",\"gone\":true}", report.token_hex);
    }
    else
    {
        len = snprintf(payload, sizeof(payload), "{\"t\":\"%s\",\"rssi\":%d,\"n\":%u}",
                       report.token_hex, report.rssi, static_cast<unsigned>(report.samples));
    }
    if (len <= 0 || static_cast<size_t>(len) >= sizeof(payload))
        return;

    char topic[128];
    construct_topic(topic, sizeof(topic), SIGHTING_ENDPOINT);
    _mqtt_client->publish(topic, reinterpret_cast<const uint8_t *>(payload), len, false);
}

void BleScannerSensor::log_stats(uint32_t now_ms)
{
    if (now_ms - _last_stats_log_ms < STATS_LOG_INTERVAL_MS)
        return;
    _last_stats_log_ms = now_ms;

    IOTCLOUD_LOG_PRINT(F("BLE: tokens="));
    IOTCLOUD_LOG_PRINT(_tracker.size());
    IOTCLOUD_LOG_PRINT(F(" drops queue="));
    IOTCLOUD_LOG_PRINT(_queue_drops.exchange(0));
    IOTCLOUD_LOG_PRINT(F(" table="));
    IOTCLOUD_LOG_PRINT(_table_drops);
    IOTCLOUD_LOG_PRINT(F(" min_free_heap="));
    IOTCLOUD_LOG_PRINTLN(ESP.getMinFreeHeap());
    _table_drops = 0;
}

#endif // ARDUINO && ESP32
