#pragma once

// Minimal HCI helpers for a scan-only device that talks to the BT controller
// directly (VHCI), with no host stack. Pure C++ so it can be tested on the host.
// Packets use the H4 format: first byte is the packet type.

#include <cstddef>
#include <cstdint>

#include "ble_beacon.h"

constexpr uint8_t BLE_HCI_COMMAND_PKT = 0x01;
constexpr uint8_t BLE_HCI_EVENT_PKT = 0x04;

constexpr uint16_t BLE_HCI_OP_RESET = 0x0C03;
constexpr uint16_t BLE_HCI_OP_SET_EVENT_MASK = 0x0C01;
constexpr uint16_t BLE_HCI_OP_LE_SET_SCAN_PARAMS = 0x200B;
constexpr uint16_t BLE_HCI_OP_LE_SET_SCAN_ENABLE = 0x200C;

// Largest command this file builds: 1 type + 2 opcode + 1 length + 8 params.
constexpr size_t BLE_HCI_MAX_COMMAND_LEN = 12;

// Called for each advertising report that carries an IotCloud beacon.
using BleBeaconSink = void (*)(void *ctx, const uint8_t token[BLE_TOKEN_LEN], int8_t rssi);

struct BleHciEventInfo
{
    bool command_done = false; // Command Complete or Command Status
    uint16_t opcode = 0;       // set when command_done
    uint8_t status = 0;        // set when command_done; 0 = success
    uint8_t adv_reports = 0;   // advertising reports read, beacon or not
};

// Parses one H4 event packet from the controller. Calls `sink` for each
// advertising report that carries a beacon. Returns false if the packet is
// not an event or is malformed (reports read before the error still count).
bool ble_hci_parse_event(const uint8_t *pkt, size_t len, BleHciEventInfo &info,
                         BleBeaconSink sink, void *ctx);

// Finds the beacon in advertising data (a list of length/type/value fields).
bool ble_ad_find_beacon(const uint8_t *ad, size_t len, uint8_t token_out[BLE_TOKEN_LEN]);

// Builds the H4 command packets used to start a passive scan. Each returns
// the packet length; `out` must hold BLE_HCI_MAX_COMMAND_LEN bytes.
size_t ble_hci_cmd_reset(uint8_t *out);
// Default event mask plus LE Meta events (bit 61), which carry adv reports.
size_t ble_hci_cmd_set_event_mask(uint8_t *out);
// Passive scan, public own address, accept all. Times in ms, interval >= window.
size_t ble_hci_cmd_le_set_scan_params(uint8_t *out, uint16_t interval_ms, uint16_t window_ms);
// Duplicate filtering off, so every packet reaches the RSSI mean.
size_t ble_hci_cmd_le_set_scan_enable(uint8_t *out, bool enable);
