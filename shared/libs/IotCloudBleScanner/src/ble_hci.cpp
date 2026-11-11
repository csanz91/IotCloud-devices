#include "ble_hci.h"

namespace
{
constexpr uint8_t EVT_COMMAND_COMPLETE = 0x0E;
constexpr uint8_t EVT_COMMAND_STATUS = 0x0F;
constexpr uint8_t EVT_LE_META = 0x3E;
constexpr uint8_t LE_SUBEVT_ADV_REPORT = 0x02;

constexpr uint8_t AD_TYPE_MANUFACTURER = 0xFF;
constexpr int8_t RSSI_UNAVAILABLE = 127;

// Event type, address type, address, data length; then data and RSSI.
constexpr size_t ADV_REPORT_HEADER_LEN = 1 + 1 + 6 + 1;

uint16_t read_u16(const uint8_t *p)
{
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

size_t build_command(uint8_t *out, uint16_t opcode, const uint8_t *params, uint8_t params_len)
{
    out[0] = BLE_HCI_COMMAND_PKT;
    out[1] = static_cast<uint8_t>(opcode & 0xFF);
    out[2] = static_cast<uint8_t>(opcode >> 8);
    out[3] = params_len;
    for (uint8_t i = 0; i < params_len; i++)
        out[4 + i] = params[i];
    return 4u + params_len;
}

uint16_t ms_to_scan_units(uint16_t ms)
{
    // Scan timing is in 0.625 ms units, valid range 0x0004..0x4000.
    uint32_t units = static_cast<uint32_t>(ms) * 8u / 5u;
    if (units < 0x0004)
        units = 0x0004;
    if (units > 0x4000)
        units = 0x4000;
    return static_cast<uint16_t>(units);
}

bool parse_adv_reports(const uint8_t *p, size_t len, BleHciEventInfo &info, BleBeaconSink sink, void *ctx)
{
    if (len < 1)
        return false;
    const uint8_t count = p[0];
    size_t pos = 1;

    // Reports are laid out one after another, as Bluedroid and NimBLE read them.
    for (uint8_t i = 0; i < count; i++)
    {
        if (len - pos < ADV_REPORT_HEADER_LEN)
            return false;
        const uint8_t data_len = p[pos + 8];
        const size_t data_pos = pos + ADV_REPORT_HEADER_LEN;
        if (len - data_pos < static_cast<size_t>(data_len) + 1) // + RSSI
            return false;

        const int8_t rssi = static_cast<int8_t>(p[data_pos + data_len]);
        info.adv_reports++;

        uint8_t token[BLE_TOKEN_LEN];
        if (rssi != RSSI_UNAVAILABLE && sink != nullptr &&
            ble_ad_find_beacon(p + data_pos, data_len, token))
        {
            sink(ctx, token, rssi);
        }
        pos = data_pos + data_len + 1;
    }
    return true;
}
} // namespace

bool ble_ad_find_beacon(const uint8_t *ad, size_t len, uint8_t token_out[BLE_TOKEN_LEN])
{
    if (ad == nullptr)
        return false;
    size_t pos = 0;
    while (pos < len)
    {
        const uint8_t field_len = ad[pos]; // type + value
        if (field_len == 0)
            return false; // padding: no more fields
        if (len - pos - 1 < field_len)
            return false; // truncated
        const uint8_t type = ad[pos + 1];
        if (type == AD_TYPE_MANUFACTURER &&
            parse_ble_beacon(ad + pos + 2, field_len - 1u, token_out))
        {
            return true;
        }
        pos += 1u + field_len;
    }
    return false;
}

bool ble_hci_parse_event(const uint8_t *pkt, size_t len, BleHciEventInfo &info,
                         BleBeaconSink sink, void *ctx)
{
    info = BleHciEventInfo();
    if (pkt == nullptr || len < 3 || pkt[0] != BLE_HCI_EVENT_PKT)
        return false;

    const uint8_t code = pkt[1];
    const uint8_t params_len = pkt[2];
    if (len - 3 < params_len)
        return false;
    const uint8_t *p = pkt + 3;

    switch (code)
    {
    case EVT_COMMAND_COMPLETE: // num packets, opcode, status, ...
        if (params_len < 4)
            return false;
        info.command_done = true;
        info.opcode = read_u16(p + 1);
        info.status = p[3];
        return true;

    case EVT_COMMAND_STATUS: // status, num packets, opcode
        if (params_len < 4)
            return false;
        info.command_done = true;
        info.status = p[0];
        info.opcode = read_u16(p + 2);
        return true;

    case EVT_LE_META:
        if (params_len < 1)
            return false;
        if (p[0] != LE_SUBEVT_ADV_REPORT)
            return true;
        return parse_adv_reports(p + 1, params_len - 1u, info, sink, ctx);

    default:
        return true;
    }
}

size_t ble_hci_cmd_reset(uint8_t *out)
{
    return build_command(out, BLE_HCI_OP_RESET, nullptr, 0);
}

size_t ble_hci_cmd_set_event_mask(uint8_t *out)
{
    // 0x20001FFFFFFFFFFF, little endian: spec default plus bit 61 (LE Meta).
    const uint8_t mask[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0x20};
    return build_command(out, BLE_HCI_OP_SET_EVENT_MASK, mask, sizeof(mask));
}

size_t ble_hci_cmd_le_set_scan_params(uint8_t *out, uint16_t interval_ms, uint16_t window_ms)
{
    const uint16_t interval = ms_to_scan_units(interval_ms);
    uint16_t window = ms_to_scan_units(window_ms);
    if (window > interval)
        window = interval;
    const uint8_t params[7] = {
        0x00, // passive
        static_cast<uint8_t>(interval & 0xFF), static_cast<uint8_t>(interval >> 8),
        static_cast<uint8_t>(window & 0xFF), static_cast<uint8_t>(window >> 8),
        0x00, // own address: public
        0x00, // accept all advertisers
    };
    return build_command(out, BLE_HCI_OP_LE_SET_SCAN_PARAMS, params, sizeof(params));
}

size_t ble_hci_cmd_le_set_scan_enable(uint8_t *out, bool enable)
{
    const uint8_t params[2] = {static_cast<uint8_t>(enable ? 1 : 0), 0x00};
    return build_command(out, BLE_HCI_OP_LE_SET_SCAN_ENABLE, params, sizeof(params));
}
