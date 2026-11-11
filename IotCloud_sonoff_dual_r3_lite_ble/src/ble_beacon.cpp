#include "ble_beacon.h"

#include <cstring>

namespace
{
constexpr uint8_t BEACON_PREFIX[] = {0xFF, 0xFF, 0x49, 0x43, 0x01}; // company FFFF, "IC", v1
constexpr size_t BEACON_PREFIX_LEN = sizeof(BEACON_PREFIX);
constexpr size_t BEACON_LEN = BEACON_PREFIX_LEN + BLE_TOKEN_LEN; // 13

constexpr int RSSI_MIN = -127;
constexpr int RSSI_MAX = 20;

int clamp_rssi(int rssi)
{
    if (rssi < RSSI_MIN)
        return RSSI_MIN;
    if (rssi > RSSI_MAX)
        return RSSI_MAX;
    return rssi;
}

// Mean rounded to the nearest integer, halves away from zero.
int rounded_mean(int32_t sum, uint16_t count)
{
    if (sum >= 0)
        return (sum + count / 2) / count;
    return -((-sum + count / 2) / count);
}
} // namespace

bool parse_ble_beacon(const uint8_t *data, size_t len, uint8_t token_out[BLE_TOKEN_LEN])
{
    if (data == nullptr || len != BEACON_LEN)
        return false;
    if (memcmp(data, BEACON_PREFIX, BEACON_PREFIX_LEN) != 0)
        return false;
    memcpy(token_out, data + BEACON_PREFIX_LEN, BLE_TOKEN_LEN);
    return true;
}

void ble_token_to_hex(const uint8_t token[BLE_TOKEN_LEN], char out[BLE_TOKEN_HEX_LEN + 1])
{
    static const char HEX_DIGITS[] = "0123456789abcdef";
    for (size_t i = 0; i < BLE_TOKEN_LEN; i++)
    {
        out[i * 2] = HEX_DIGITS[token[i] >> 4];
        out[i * 2 + 1] = HEX_DIGITS[token[i] & 0x0F];
    }
    out[BLE_TOKEN_HEX_LEN] = '\0';
}

bool BleSightingTracker::add_sample(const uint8_t token[BLE_TOKEN_LEN], int rssi, uint32_t now_ms)
{
    Entry *free_entry = nullptr;
    Entry *entry = nullptr;
    for (Entry &e : _entries)
    {
        if (e.used && memcmp(e.token, token, BLE_TOKEN_LEN) == 0)
        {
            entry = &e;
            break;
        }
        if (!e.used && free_entry == nullptr)
            free_entry = &e;
    }

    if (entry == nullptr)
    {
        if (free_entry == nullptr)
            return false;
        entry = free_entry;
        *entry = Entry();
        entry->used = true;
        memcpy(entry->token, token, BLE_TOKEN_LEN);
    }

    if (entry->samples == 0)
        entry->window_start_ms = now_ms;
    if (entry->samples < UINT16_MAX)
    {
        entry->rssi_sum += clamp_rssi(rssi);
        entry->samples++;
    }
    entry->last_heard_ms = now_ms;
    return true;
}

bool BleSightingTracker::poll(uint32_t now_ms, BleSightingReport &out)
{
    for (Entry &e : _entries)
    {
        if (!e.used)
            continue;

        if (e.samples > 0 && now_ms - e.window_start_ms >= REPORT_INTERVAL_MS)
        {
            ble_token_to_hex(e.token, out.token_hex);
            out.rssi = clamp_rssi(rounded_mean(e.rssi_sum, e.samples));
            out.samples = e.samples;
            out.gone = false;
            e.rssi_sum = 0;
            e.samples = 0;
            return true;
        }

        if (now_ms - e.last_heard_ms >= GONE_TIMEOUT_MS)
        {
            ble_token_to_hex(e.token, out.token_hex);
            out.rssi = 0;
            out.samples = 0;
            out.gone = true;
            e = Entry();
            return true;
        }
    }
    return false;
}

size_t BleSightingTracker::size() const
{
    size_t count = 0;
    for (const Entry &e : _entries)
    {
        if (e.used)
            count++;
    }
    return count;
}
