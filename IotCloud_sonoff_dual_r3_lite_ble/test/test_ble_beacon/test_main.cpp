#include <cstring>
#include <unity.h>

#include "ble_beacon.h"
#include "ble_hci.h"
#include <vector>

namespace
{
// Token f092b13c7e9cbc35 (test vector, slot 1900000).
const uint8_t TOKEN_A[BLE_TOKEN_LEN] = {0xf0, 0x92, 0xb1, 0x3c, 0x7e, 0x9c, 0xbc, 0x35};
const uint8_t TOKEN_B[BLE_TOKEN_LEN] = {0x14, 0x45, 0xf6, 0x80, 0xe2, 0xde, 0x4b, 0x1e};

const uint8_t VALID_BEACON[] = {0xFF, 0xFF, 0x49, 0x43, 0x01,
                                0xf0, 0x92, 0xb1, 0x3c, 0x7e, 0x9c, 0xbc, 0x35};
} // namespace

void setUp() {}
void tearDown() {}

void test_parse_valid_beacon()
{
    uint8_t token[BLE_TOKEN_LEN] = {};
    TEST_ASSERT_TRUE(parse_ble_beacon(VALID_BEACON, sizeof(VALID_BEACON), token));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(TOKEN_A, token, BLE_TOKEN_LEN);
}

void test_parse_rejects_wrong_length()
{
    uint8_t token[BLE_TOKEN_LEN];
    uint8_t longer[sizeof(VALID_BEACON) + 1];
    memcpy(longer, VALID_BEACON, sizeof(VALID_BEACON));
    longer[sizeof(VALID_BEACON)] = 0;

    TEST_ASSERT_FALSE(parse_ble_beacon(VALID_BEACON, sizeof(VALID_BEACON) - 1, token));
    TEST_ASSERT_FALSE(parse_ble_beacon(longer, sizeof(longer), token));
    TEST_ASSERT_FALSE(parse_ble_beacon(nullptr, sizeof(VALID_BEACON), token));
}

void test_parse_rejects_wrong_prefix()
{
    uint8_t token[BLE_TOKEN_LEN];
    for (size_t i = 0; i < 5; i++)
    {
        uint8_t data[sizeof(VALID_BEACON)];
        memcpy(data, VALID_BEACON, sizeof(data));
        data[i] ^= 0x01; // company ID, magic or version
        TEST_ASSERT_FALSE(parse_ble_beacon(data, sizeof(data), token));
    }
}

void test_token_hex_is_lowercase()
{
    char hex[BLE_TOKEN_HEX_LEN + 1];
    ble_token_to_hex(TOKEN_A, hex);
    TEST_ASSERT_EQUAL_STRING("f092b13c7e9cbc35", hex);
}

void test_reports_mean_after_two_seconds()
{
    BleSightingTracker tracker;
    BleSightingReport report;

    tracker.add_sample(TOKEN_A, -60, 1000);
    tracker.add_sample(TOKEN_A, -65, 1500);
    tracker.add_sample(TOKEN_A, -63, 2900);
    TEST_ASSERT_FALSE(tracker.poll(2999, report));

    TEST_ASSERT_TRUE(tracker.poll(3000, report));
    TEST_ASSERT_EQUAL_STRING("f092b13c7e9cbc35", report.token_hex);
    TEST_ASSERT_EQUAL_INT(-63, report.rssi); // -188 / 3 = -62.67
    TEST_ASSERT_EQUAL_UINT16(3, report.samples);
    TEST_ASSERT_FALSE(report.gone);
    TEST_ASSERT_FALSE(tracker.poll(3000, report));
}

void test_at_most_one_report_every_two_seconds()
{
    BleSightingTracker tracker;
    BleSightingReport report;

    tracker.add_sample(TOKEN_A, -70, 0);
    TEST_ASSERT_TRUE(tracker.poll(2000, report));

    // The next window starts with the next sample.
    tracker.add_sample(TOKEN_A, -71, 2100);
    TEST_ASSERT_FALSE(tracker.poll(4000, report));
    TEST_ASSERT_TRUE(tracker.poll(4100, report));
    TEST_ASSERT_EQUAL_INT(-71, report.rssi);
    TEST_ASSERT_EQUAL_UINT16(1, report.samples);
}

void test_gone_after_twenty_seconds_of_silence()
{
    BleSightingTracker tracker;
    BleSightingReport report;

    tracker.add_sample(TOKEN_A, -80, 5000);
    TEST_ASSERT_TRUE(tracker.poll(7000, report));
    TEST_ASSERT_FALSE(tracker.poll(24999, report));

    TEST_ASSERT_TRUE(tracker.poll(25000, report));
    TEST_ASSERT_TRUE(report.gone);
    TEST_ASSERT_EQUAL_STRING("f092b13c7e9cbc35", report.token_hex);
    TEST_ASSERT_EQUAL_size_t(0, tracker.size());
    TEST_ASSERT_FALSE(tracker.poll(60000, report)); // forgotten: only one gone
}

void test_mean_is_flushed_before_gone()
{
    BleSightingTracker tracker;
    BleSightingReport report;

    tracker.add_sample(TOKEN_A, -75, 0);
    // loop() stalled for a long time: the pending mean still goes out first.
    TEST_ASSERT_TRUE(tracker.poll(30000, report));
    TEST_ASSERT_FALSE(report.gone);
    TEST_ASSERT_TRUE(tracker.poll(30000, report));
    TEST_ASSERT_TRUE(report.gone);
}

void test_tokens_are_tracked_separately()
{
    BleSightingTracker tracker;
    BleSightingReport report;

    tracker.add_sample(TOKEN_A, -50, 0);
    tracker.add_sample(TOKEN_B, -90, 1000);
    TEST_ASSERT_EQUAL_size_t(2, tracker.size());

    TEST_ASSERT_TRUE(tracker.poll(2000, report));
    TEST_ASSERT_EQUAL_STRING("f092b13c7e9cbc35", report.token_hex);
    TEST_ASSERT_EQUAL_INT(-50, report.rssi);
    TEST_ASSERT_FALSE(tracker.poll(2000, report));

    TEST_ASSERT_TRUE(tracker.poll(3000, report));
    TEST_ASSERT_EQUAL_STRING("1445f680e2de4b1e", report.token_hex);
    TEST_ASSERT_EQUAL_INT(-90, report.rssi);
}

void test_rssi_is_clamped()
{
    BleSightingTracker tracker;
    BleSightingReport report;

    tracker.add_sample(TOKEN_A, 50, 0);
    tracker.add_sample(TOKEN_B, -200, 0);
    TEST_ASSERT_TRUE(tracker.poll(2000, report));
    TEST_ASSERT_EQUAL_INT(20, report.rssi);
    TEST_ASSERT_TRUE(tracker.poll(2000, report));
    TEST_ASSERT_EQUAL_INT(-127, report.rssi);
}

void test_full_table_drops_new_tokens()
{
    BleSightingTracker tracker;
    uint8_t token[BLE_TOKEN_LEN] = {};
    for (size_t i = 0; i < BleSightingTracker::MAX_TOKENS; i++)
    {
        token[0] = static_cast<uint8_t>(i);
        TEST_ASSERT_TRUE(tracker.add_sample(token, -70, 0));
    }
    token[0] = 0xEE;
    TEST_ASSERT_FALSE(tracker.add_sample(token, -70, 0));

    // Known tokens still update.
    token[0] = 0;
    TEST_ASSERT_TRUE(tracker.add_sample(token, -70, 100));
}

void test_millis_wraparound()
{
    BleSightingTracker tracker;
    BleSightingReport report;

    const uint32_t start = UINT32_MAX - 500;
    tracker.add_sample(TOKEN_A, -60, start);
    TEST_ASSERT_FALSE(tracker.poll(start + 1999, report));
    TEST_ASSERT_TRUE(tracker.poll(start + 2000, report)); // wrapped past 0
    TEST_ASSERT_FALSE(report.gone);
    TEST_ASSERT_TRUE(tracker.poll(start + 20000, report));
    TEST_ASSERT_TRUE(report.gone);
}

// --- HCI / advertising data -------------------------------------------------

namespace
{
struct Seen
{
    int calls = 0;
    uint8_t token[BLE_TOKEN_LEN] = {};
    int8_t rssi = 0;
};

void record(void *ctx, const uint8_t token[BLE_TOKEN_LEN], int8_t rssi)
{
    Seen *seen = static_cast<Seen *>(ctx);
    seen->calls++;
    memcpy(seen->token, token, BLE_TOKEN_LEN);
    seen->rssi = rssi;
}

// Flags field, then the beacon as a manufacturer field (length 14, type FF).
std::vector<uint8_t> beacon_ad()
{
    std::vector<uint8_t> ad = {0x02, 0x01, 0x06, 0x0E, 0xFF};
    ad.insert(ad.end(), VALID_BEACON, VALID_BEACON + sizeof(VALID_BEACON));
    return ad;
}

// One LE Advertising Report event per report: {ad, rssi}.
std::vector<uint8_t> adv_event(const std::vector<std::pair<std::vector<uint8_t>, int8_t>> &reports)
{
    std::vector<uint8_t> params = {0x02, static_cast<uint8_t>(reports.size())};
    for (const auto &r : reports)
    {
        params.push_back(0x03); // ADV_NONCONN_IND
        params.push_back(0x01); // random address
        for (int i = 0; i < 6; i++)
            params.push_back(static_cast<uint8_t>(0xA0 + i));
        params.push_back(static_cast<uint8_t>(r.first.size()));
        params.insert(params.end(), r.first.begin(), r.first.end());
        params.push_back(static_cast<uint8_t>(r.second));
    }
    std::vector<uint8_t> pkt = {0x04, 0x3E, static_cast<uint8_t>(params.size())};
    pkt.insert(pkt.end(), params.begin(), params.end());
    return pkt;
}
} // namespace

void test_ad_finds_beacon_after_other_fields()
{
    const std::vector<uint8_t> ad = beacon_ad();
    uint8_t token[BLE_TOKEN_LEN] = {};
    TEST_ASSERT_TRUE(ble_ad_find_beacon(ad.data(), ad.size(), token));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(TOKEN_A, token, BLE_TOKEN_LEN);
}

void test_ad_rejects_truncated_field()
{
    std::vector<uint8_t> ad = beacon_ad();
    ad.pop_back();
    uint8_t token[BLE_TOKEN_LEN];
    TEST_ASSERT_FALSE(ble_ad_find_beacon(ad.data(), ad.size(), token));
}

void test_ad_ignores_other_manufacturer_data()
{
    // Apple iBeacon-style manufacturer field: company 0x004C.
    const uint8_t ad[] = {0x05, 0xFF, 0x4C, 0x00, 0x02, 0x15};
    uint8_t token[BLE_TOKEN_LEN];
    TEST_ASSERT_FALSE(ble_ad_find_beacon(ad, sizeof(ad), token));
}

void test_hci_adv_report_calls_sink()
{
    const std::vector<uint8_t> pkt = adv_event({{beacon_ad(), -63}});
    Seen seen;
    BleHciEventInfo info;
    TEST_ASSERT_TRUE(ble_hci_parse_event(pkt.data(), pkt.size(), info, record, &seen));
    TEST_ASSERT_EQUAL(1, info.adv_reports);
    TEST_ASSERT_FALSE(info.command_done);
    TEST_ASSERT_EQUAL(1, seen.calls);
    TEST_ASSERT_EQUAL_INT8(-63, seen.rssi);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(TOKEN_A, seen.token, BLE_TOKEN_LEN);
}

void test_hci_multiple_reports_and_non_beacons()
{
    const std::vector<uint8_t> other = {0x02, 0x01, 0x06};
    const std::vector<uint8_t> pkt = adv_event({{other, -40}, {beacon_ad(), -70}});
    Seen seen;
    BleHciEventInfo info;
    TEST_ASSERT_TRUE(ble_hci_parse_event(pkt.data(), pkt.size(), info, record, &seen));
    TEST_ASSERT_EQUAL(2, info.adv_reports);
    TEST_ASSERT_EQUAL(1, seen.calls);
    TEST_ASSERT_EQUAL_INT8(-70, seen.rssi);
}

void test_hci_skips_unavailable_rssi()
{
    const std::vector<uint8_t> pkt = adv_event({{beacon_ad(), 127}});
    Seen seen;
    BleHciEventInfo info;
    TEST_ASSERT_TRUE(ble_hci_parse_event(pkt.data(), pkt.size(), info, record, &seen));
    TEST_ASSERT_EQUAL(1, info.adv_reports);
    TEST_ASSERT_EQUAL(0, seen.calls);
}

void test_hci_rejects_truncated_report()
{
    std::vector<uint8_t> pkt = adv_event({{beacon_ad(), -63}});
    pkt.pop_back(); // drop RSSI
    pkt[2] = static_cast<uint8_t>(pkt.size() - 3);
    Seen seen;
    BleHciEventInfo info;
    TEST_ASSERT_FALSE(ble_hci_parse_event(pkt.data(), pkt.size(), info, record, &seen));
    TEST_ASSERT_EQUAL(0, seen.calls);
}

void test_hci_command_complete_and_status()
{
    // Command Complete for LE Set Scan Enable, status 0.
    const uint8_t complete[] = {0x04, 0x0E, 0x04, 0x01, 0x0C, 0x20, 0x00};
    BleHciEventInfo info;
    TEST_ASSERT_TRUE(ble_hci_parse_event(complete, sizeof(complete), info, nullptr, nullptr));
    TEST_ASSERT_TRUE(info.command_done);
    TEST_ASSERT_EQUAL_HEX16(BLE_HCI_OP_LE_SET_SCAN_ENABLE, info.opcode);
    TEST_ASSERT_EQUAL_HEX8(0x00, info.status);

    // Command Status for Reset, status 0x0C (command disallowed).
    const uint8_t status[] = {0x04, 0x0F, 0x04, 0x0C, 0x01, 0x03, 0x0C};
    TEST_ASSERT_TRUE(ble_hci_parse_event(status, sizeof(status), info, nullptr, nullptr));
    TEST_ASSERT_TRUE(info.command_done);
    TEST_ASSERT_EQUAL_HEX16(BLE_HCI_OP_RESET, info.opcode);
    TEST_ASSERT_EQUAL_HEX8(0x0C, info.status);
}

void test_hci_commands_are_encoded()
{
    uint8_t cmd[BLE_HCI_MAX_COMMAND_LEN];

    const uint8_t reset[] = {0x01, 0x03, 0x0C, 0x00};
    TEST_ASSERT_EQUAL(sizeof(reset), ble_hci_cmd_reset(cmd));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reset, cmd, sizeof(reset));

    const uint8_t mask[] = {0x01, 0x01, 0x0C, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0x20};
    TEST_ASSERT_EQUAL(sizeof(mask), ble_hci_cmd_set_event_mask(cmd));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mask, cmd, sizeof(mask));

    // 100 ms = 160 units (0x00A0), 50 ms = 80 units (0x0050), passive.
    const uint8_t params[] = {0x01, 0x0B, 0x20, 0x07, 0x00, 0xA0, 0x00, 0x50, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL(sizeof(params), ble_hci_cmd_le_set_scan_params(cmd, 100, 50));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(params, cmd, sizeof(params));

    // Window longer than interval is clamped to the interval.
    ble_hci_cmd_le_set_scan_params(cmd, 100, 200);
    TEST_ASSERT_EQUAL_HEX8(0xA0, cmd[7]);

    const uint8_t enable[] = {0x01, 0x0C, 0x20, 0x02, 0x01, 0x00};
    TEST_ASSERT_EQUAL(sizeof(enable), ble_hci_cmd_le_set_scan_enable(cmd, true));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(enable, cmd, sizeof(enable));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_valid_beacon);
    RUN_TEST(test_parse_rejects_wrong_length);
    RUN_TEST(test_parse_rejects_wrong_prefix);
    RUN_TEST(test_token_hex_is_lowercase);
    RUN_TEST(test_reports_mean_after_two_seconds);
    RUN_TEST(test_at_most_one_report_every_two_seconds);
    RUN_TEST(test_gone_after_twenty_seconds_of_silence);
    RUN_TEST(test_mean_is_flushed_before_gone);
    RUN_TEST(test_tokens_are_tracked_separately);
    RUN_TEST(test_rssi_is_clamped);
    RUN_TEST(test_full_table_drops_new_tokens);
    RUN_TEST(test_millis_wraparound);
    RUN_TEST(test_ad_finds_beacon_after_other_fields);
    RUN_TEST(test_ad_rejects_truncated_field);
    RUN_TEST(test_ad_ignores_other_manufacturer_data);
    RUN_TEST(test_hci_adv_report_calls_sink);
    RUN_TEST(test_hci_multiple_reports_and_non_beacons);
    RUN_TEST(test_hci_skips_unavailable_rssi);
    RUN_TEST(test_hci_rejects_truncated_report);
    RUN_TEST(test_hci_command_complete_and_status);
    RUN_TEST(test_hci_commands_are_encoded);
    return UNITY_END();
}
