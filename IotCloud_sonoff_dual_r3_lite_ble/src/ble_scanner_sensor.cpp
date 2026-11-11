#include "ble_scanner_sensor.h"

#include <NimBLEDevice.h>

namespace
{
constexpr char SIGHTING_ENDPOINT[] = "aux/ble/sighting";

class BeaconScanCallbacks : public NimBLEScanCallbacks
{
public:
    explicit BeaconScanCallbacks(BleScannerSensor &sensor) : _sensor(sensor) {}

    void onResult(const NimBLEAdvertisedDevice *device) override
    {
        uint8_t token[BLE_TOKEN_LEN];
        const uint8_t count = device->getManufacturerDataCount();
        for (uint8_t i = 0; i < count; i++)
        {
            const std::string data = device->getManufacturerData(i);
            if (parse_ble_beacon(reinterpret_cast<const uint8_t *>(data.data()), data.size(), token))
            {
                _sensor.enqueue_sample(token, device->getRSSI());
                return;
            }
        }
    }

    // A continuous scan shouldn't end; loop() restarts it if it does.
    void onScanEnd(const NimBLEScanResults &, int reason) override
    {
        IOTCLOUD_LOG_PRINT(F("BLE scan ended, reason "));
        IOTCLOUD_LOG_PRINTLN(reason);
    }

private:
    BleScannerSensor &_sensor;
};

BeaconScanCallbacks *scan_callbacks = nullptr;
} // namespace

BleScannerSensor::BleScannerSensor(const char *sensor_id, const char *sensor_name)
    : BaseSensor(sensor_id, sensor_name, "ble_scanner", "{}")
{
}

void BleScannerSensor::begin_scanning()
{
    if (_scanning)
        return;

    _queue = xQueueCreate(QUEUE_LENGTH, sizeof(Sample));
    if (_queue == nullptr)
    {
        IOTCLOUD_LOG_PRINTLN(F("BLE: queue allocation failed"));
        return;
    }

    NimBLEDevice::init("");
    scan_callbacks = new BeaconScanCallbacks(*this);

    NimBLEScan *scan = NimBLEDevice::getScan();
    // wantDuplicates=true turns the controller duplicate filter off, so every
    // packet updates the RSSI mean.
    scan->setScanCallbacks(scan_callbacks, true);
    scan->setActiveScan(false);
    scan->setInterval(BLE_SCAN_INTERVAL_MS);
    scan->setWindow(BLE_SCAN_WINDOW_MS);
    // Don't keep results: phones randomize their MAC, so the list would grow forever.
    scan->setMaxResults(0);

    _scanning = true;
    _last_scan_check_ms = millis();
    if (!scan->start(0, false, true))
    {
        IOTCLOUD_LOG_PRINTLN(F("BLE: scan start failed, will retry"));
    }
}

void BleScannerSensor::enqueue_sample(const uint8_t token[BLE_TOKEN_LEN], int8_t rssi)
{
    Sample sample;
    memcpy(sample.token, token, BLE_TOKEN_LEN);
    sample.rssi = rssi;
    if (xQueueSend(_queue, &sample, 0) != pdTRUE)
    {
        _queue_drops = _queue_drops + 1;
    }
}

void BleScannerSensor::loop()
{
    if (!_scanning)
        return;

    const uint32_t now = millis();

    Sample sample;
    while (xQueueReceive(_queue, &sample, 0) == pdTRUE)
    {
        if (!_tracker.add_sample(sample.token, sample.rssi, now))
        {
            _table_drops++;
        }
    }

    BleSightingReport report;
    while (_tracker.poll(now, report))
    {
        publish(report);
    }

    ensure_scanning(now);
    log_drops(now);
}

void BleScannerSensor::ensure_scanning(uint32_t now_ms)
{
    if (now_ms - _last_scan_check_ms < SCAN_CHECK_INTERVAL_MS)
        return;
    _last_scan_check_ms = now_ms;

    NimBLEScan *scan = NimBLEDevice::getScan();
    if (!scan->isScanning())
    {
        IOTCLOUD_LOG_PRINTLN(F("BLE: scan stopped, restarting"));
        scan->start(0, false, true);
    }
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

void BleScannerSensor::log_drops(uint32_t now_ms)
{
    if (now_ms - _last_drop_log_ms < DROP_LOG_INTERVAL_MS)
        return;
    _last_drop_log_ms = now_ms;

    const uint32_t queue_drops = _queue_drops;
    if (queue_drops == 0 && _table_drops == 0)
        return;

    IOTCLOUD_LOG_PRINT(F("BLE: dropped samples, queue="));
    IOTCLOUD_LOG_PRINT(queue_drops);
    IOTCLOUD_LOG_PRINT(F(" table="));
    IOTCLOUD_LOG_PRINTLN(_table_drops);
    _queue_drops = 0;
    _table_drops = 0;
}
