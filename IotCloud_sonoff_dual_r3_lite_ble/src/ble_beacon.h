#pragma once

// Pure C++ (no Arduino) so it can be unit tested on the host.
// Protocol: see README.md, "BLE presence beacon protocol v1".

#include <cstddef>
#include <cstdint>

constexpr size_t BLE_TOKEN_LEN = 8;
constexpr size_t BLE_TOKEN_HEX_LEN = BLE_TOKEN_LEN * 2;

// Returns true if `data` is an IotCloud beacon manufacturer data field
// (company ID FF FF, "IC", version 1, 8-byte token) and copies the token.
// `data` includes the 2-byte company ID, as NimBLE returns it.
bool parse_ble_beacon(const uint8_t *data, size_t len, uint8_t token_out[BLE_TOKEN_LEN]);

void ble_token_to_hex(const uint8_t token[BLE_TOKEN_LEN], char out[BLE_TOKEN_HEX_LEN + 1]);

struct BleSightingReport
{
    char token_hex[BLE_TOKEN_HEX_LEN + 1];
    int rssi;         // mean dBm over the window, clamped to -127..20
    uint16_t samples; // number of packets averaged
    bool gone;        // true: token not heard for GONE_TIMEOUT_MS
};

// Tracks each token separately: averages RSSI over windows of at least
// REPORT_INTERVAL_MS and reports `gone` after GONE_TIMEOUT_MS of silence.
// Presence decisions are left to the backend.
class BleSightingTracker
{
public:
    static constexpr size_t MAX_TOKENS = 16;
    static constexpr uint32_t REPORT_INTERVAL_MS = 2000;
    static constexpr uint32_t GONE_TIMEOUT_MS = 20000;

    // Returns false if the token is new and the table is full.
    bool add_sample(const uint8_t token[BLE_TOKEN_LEN], int rssi, uint32_t now_ms);

    // Fills `out` with the next due report. Call until it returns false.
    bool poll(uint32_t now_ms, BleSightingReport &out);

    size_t size() const;

private:
    struct Entry
    {
        bool used = false;
        uint8_t token[BLE_TOKEN_LEN] = {};
        int32_t rssi_sum = 0;
        uint16_t samples = 0;
        uint32_t window_start_ms = 0;
        uint32_t last_heard_ms = 0;
    };

    Entry _entries[MAX_TOKENS];
};
