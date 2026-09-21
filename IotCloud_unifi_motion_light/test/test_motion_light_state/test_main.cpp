#include <cstdint>
#include <limits>

#include <unity.h>

#include "motion_light_state.h"

void setUp()
{
}

void tearDown()
{
}

void test_initial_state_is_off()
{
    MotionLightState state;

    TEST_ASSERT_FALSE(state.is_on());
    TEST_ASSERT_FALSE(state.motion_timer_active());
    TEST_ASSERT_EQUAL_UINT32(30, state.timeout_seconds());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(MotionLightState::Mode::OFF),
        static_cast<int>(state.mode()));
}

void test_motion_turns_on_and_expires_after_default_timeout()
{
    MotionLightState state;

    TEST_ASSERT_TRUE(state.trigger_motion(1000));
    TEST_ASSERT_TRUE(state.is_on());
    TEST_ASSERT_FALSE(state.tick(30999));
    TEST_ASSERT_TRUE(state.is_on());
    TEST_ASSERT_TRUE(state.tick(31000));
    TEST_ASSERT_FALSE(state.is_on());
}

void test_repeated_motion_extends_the_timer()
{
    MotionLightState state;

    state.trigger_motion(0);
    TEST_ASSERT_FALSE(state.trigger_motion(25000));
    TEST_ASSERT_FALSE(state.tick(54999));
    TEST_ASSERT_TRUE(state.is_on());
    TEST_ASSERT_TRUE(state.tick(55000));
    TEST_ASSERT_FALSE(state.is_on());
}

void test_manual_on_latches_and_manual_off_allows_future_motion()
{
    MotionLightState state;

    state.trigger_motion(0);
    TEST_ASSERT_FALSE(state.manual_set(true));
    TEST_ASSERT_FALSE(state.motion_timer_active());
    TEST_ASSERT_FALSE(state.tick(60000));
    TEST_ASSERT_TRUE(state.is_on());

    TEST_ASSERT_TRUE(state.manual_set(false));
    TEST_ASSERT_FALSE(state.is_on());
    TEST_ASSERT_TRUE(state.trigger_motion(70000));
    TEST_ASSERT_TRUE(state.motion_timer_active());
}

void test_motion_does_not_arm_timer_while_manually_on()
{
    MotionLightState state;

    TEST_ASSERT_TRUE(state.manual_set(true));
    TEST_ASSERT_FALSE(state.trigger_motion(1000));
    TEST_ASSERT_FALSE(state.motion_timer_active());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(MotionLightState::Mode::MANUAL_ON),
        static_cast<int>(state.mode()));
    TEST_ASSERT_FALSE(state.tick(60000));
    TEST_ASSERT_TRUE(state.is_on());
}

void test_manual_toggle_uses_current_output_state()
{
    MotionLightState state;

    TEST_ASSERT_TRUE(state.manual_toggle());
    TEST_ASSERT_TRUE(state.is_on());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(MotionLightState::Mode::MANUAL_ON),
        static_cast<int>(state.mode()));
    TEST_ASSERT_TRUE(state.manual_toggle());
    TEST_ASSERT_FALSE(state.is_on());
}

void test_invalid_timeout_values_are_rejected()
{
    MotionLightState state;

    TEST_ASSERT_FALSE(state.set_timeout_seconds(9));
    TEST_ASSERT_FALSE(state.set_timeout_seconds(1801));
    TEST_ASSERT_EQUAL_UINT32(30, state.timeout_seconds());
    TEST_ASSERT_TRUE(state.set_timeout_seconds(10));
    TEST_ASSERT_TRUE(state.set_timeout_seconds(1800));
    TEST_ASSERT_EQUAL_UINT32(1800, state.timeout_seconds());
}

void test_timeout_change_applies_to_the_next_motion_trigger()
{
    MotionLightState state;

    state.trigger_motion(0);
    TEST_ASSERT_TRUE(state.set_timeout_seconds(10));
    TEST_ASSERT_FALSE(state.tick(10000));
    TEST_ASSERT_TRUE(state.tick(30000));

    state.trigger_motion(40000);
    TEST_ASSERT_FALSE(state.tick(49999));
    TEST_ASSERT_TRUE(state.tick(50000));
}

void test_timeout_is_safe_across_millis_rollover()
{
    MotionLightState state;
    TEST_ASSERT_TRUE(state.set_timeout_seconds(10));

    const uint32_t started_at = std::numeric_limits<uint32_t>::max() - 5000;
    state.trigger_motion(started_at);

    TEST_ASSERT_FALSE(state.tick(started_at + 9999));
    TEST_ASSERT_TRUE(state.is_on());
    TEST_ASSERT_TRUE(state.tick(started_at + 10000));
    TEST_ASSERT_FALSE(state.is_on());
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_initial_state_is_off);
    RUN_TEST(test_motion_turns_on_and_expires_after_default_timeout);
    RUN_TEST(test_repeated_motion_extends_the_timer);
    RUN_TEST(test_manual_on_latches_and_manual_off_allows_future_motion);
    RUN_TEST(test_motion_does_not_arm_timer_while_manually_on);
    RUN_TEST(test_manual_toggle_uses_current_output_state);
    RUN_TEST(test_invalid_timeout_values_are_rejected);
    RUN_TEST(test_timeout_change_applies_to_the_next_motion_trigger);
    RUN_TEST(test_timeout_is_safe_across_millis_rollover);
    return UNITY_END();
}
