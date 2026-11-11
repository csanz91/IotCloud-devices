#include <cstring>
#include <unity.h>

#include "ble_beacon.h"

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
    return UNITY_END();
}
