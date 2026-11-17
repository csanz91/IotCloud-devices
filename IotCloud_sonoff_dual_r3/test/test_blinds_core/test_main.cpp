// Host tests for the Sonoff Dual R3 blinds logic: pio test -e native
//
// The simulator ticks the controller every millisecond (like loop() on the
// device) and checks the relay safety invariants on every single tick:
//   - both relays are never on at the same time;
//   - a relay never turns on less than the reversal dead time after the
//     opposite relay turned off.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <unity.h>

#include "blinds_core.h"

namespace
{
constexpr uint32_t DEBOUNCE = 80;
constexpr uint32_t BLANKING = 150;
constexpr uint32_t DEAD_TIME = 500;
constexpr uint32_t SWITCH_CAP = 60000;
constexpr uint32_t FULL_OPEN = 20000;

// Deterministic PRNG, so a failing noise test always fails the same way.
struct Rng
{
    uint32_t state;
    explicit Rng(uint32_t seed) : state(seed ? seed : 1) {}
    uint32_t next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    bool chance(uint32_t percent) { return next() % 100 < percent; }
};

enum class NoiseMode
{
    NONE,
    // Inputs read random values for `noise_ms` after every relay change.
    RANDOM_AFTER_RELAY_CHANGE,
    // Inputs read "pressed" for `noise_ms` after every relay change: the
    // worst case, because it looks exactly like a real press.
    STUCK_PRESSED_AFTER_RELAY_CHANGE,
};

struct Sim
{
    BlindsController blinds;
    uint32_t now;
    bool up_pressed = false;   // what the user is physically doing
    bool down_pressed = false;

    NoiseMode noise = NoiseMode::NONE;
    uint32_t noise_ms = 0;
    bool noise_on_up = true;
    bool noise_on_down = true;
    Rng rng{12345};
    uint32_t last_relay_change = 0;
    bool any_relay_change = false;

    bool prev_up = false;
    bool prev_down = false;
    bool up_ever_off = false;
    bool down_ever_off = false;
    uint32_t up_off_at = 0;
    uint32_t down_off_at = 0;
    int relay_transitions = 0;
    int starts = 0;

    explicit Sim(uint32_t start = 1000, bool up_at_boot = false, bool down_at_boot = false)
        : blinds(), now(start), up_pressed(up_at_boot), down_pressed(down_at_boot)
    {
        blinds.begin(now, up_pressed, down_pressed);
    }

    bool noisy() const
    {
        return noise != NoiseMode::NONE && any_relay_change && now - last_relay_change < noise_ms;
    }

    bool raw(bool real, bool affected)
    {
        if (!affected || !noisy())
        {
            return real;
        }
        if (noise == NoiseMode::STUCK_PRESSED_AFTER_RELAY_CHANGE)
        {
            return true;
        }
        return rng.chance(50);
    }

    void check_invariants()
    {
        const bool up = blinds.relay_up();
        const bool down = blinds.relay_down();

        char msg[96];
        snprintf(msg, sizeof(msg), "both relays on at t=%lu", (unsigned long)now);
        TEST_ASSERT_FALSE_MESSAGE(up && down, msg);

        if (up && !prev_up)
        {
            starts++;
            if (down_ever_off)
            {
                snprintf(msg, sizeof(msg), "UP energized %lu ms after DOWN off",
                         (unsigned long)(now - down_off_at));
                TEST_ASSERT_TRUE_MESSAGE(now - down_off_at >= DEAD_TIME, msg);
            }
        }
        if (down && !prev_down)
        {
            starts++;
            if (up_ever_off)
            {
                snprintf(msg, sizeof(msg), "DOWN energized %lu ms after UP off",
                         (unsigned long)(now - up_off_at));
                TEST_ASSERT_TRUE_MESSAGE(now - up_off_at >= DEAD_TIME, msg);
            }
        }
        if (!up && prev_up)
        {
            up_ever_off = true;
            up_off_at = now;
        }
        if (!down && prev_down)
        {
            down_ever_off = true;
            down_off_at = now;
        }
        if (up != prev_up || down != prev_down)
        {
            relay_transitions += (up != prev_up) + (down != prev_down);
            last_relay_change = now;
            any_relay_change = true;
        }
        prev_up = up;
        prev_down = down;
    }

    // 0 = tick every 1 ms. Otherwise each tick advances a random 3..max ms,
    // like a real loop() whose cadence varies with WiFi/MQTT work.
    uint32_t jitter_max_ms = 0;
    Rng jitter{4242};

    void step()
    {
        now += jitter_max_ms == 0 ? 1 : 3 + jitter.next() % (jitter_max_ms - 2);
        blinds.tick(now, raw(up_pressed, noise_on_up), raw(down_pressed, noise_on_down));
        check_invariants();
    }

    void run(uint32_t ms)
    {
        const uint32_t end = now + ms;
        while (static_cast<int32_t>(end - now) > 0)
        {
            step();
        }
    }

    // Applies a remote command and checks invariants immediately (the device
    // writes the relays right after handling the MQTT message).
    void command(float percent)
    {
        blinds.command_movement(percent, now);
        check_invariants();
    }

    BlindsDirection dir() const { return blinds.direction(); }
};

void assert_dir(BlindsDirection expected, BlindsDirection actual, const char *what)
{
    TEST_ASSERT_EQUAL_INT_MESSAGE(static_cast<int>(expected), static_cast<int>(actual), what);
}
} // namespace

void setUp() {}
void tearDown() {}

// ---------------------------------------------------------------------------
// Wall switch: hold-to-move

void test_switch_press_moves_until_release_not_a_fixed_time()
{
    Sim sim;
    sim.up_pressed = true;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "moving after debounce");

    // Way past every configured travel time (old firmware stopped at 10 s).
    sim.run(40000);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "still moving while held");

    sim.up_pressed = false;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped after release");
}

void test_down_switch_moves_until_release()
{
    Sim sim;
    sim.down_pressed = true;
    sim.run(3000);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "closing while held");
    TEST_ASSERT_TRUE(sim.blinds.relay_down());
    TEST_ASSERT_FALSE(sim.blinds.relay_up());

    sim.down_pressed = false;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped after release");
}

void test_press_is_accepted_only_after_debounce_time()
{
    Sim sim;
    sim.up_pressed = true;
    sim.run(DEBOUNCE - 1);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "not yet");
    sim.run(2);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "accepted");
}

void test_held_switch_is_capped_by_safety_timeout()
{
    Sim sim;
    sim.up_pressed = true;
    sim.run(DEBOUNCE + 1);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "moving");
    sim.run(SWITCH_CAP - 10);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "still moving before cap");
    sim.run(20);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "cap reached");

    // Still held: it must not restart on its own (latched switch, stuck input).
    sim.run(120000);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "no restart while still held");
    TEST_ASSERT_EQUAL_INT(1, sim.starts);

    // A new press works.
    sim.up_pressed = false;
    sim.run(500);
    sim.up_pressed = true;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "new press moves");
}

void test_switch_latched_at_boot_does_not_move()
{
    Sim sim(1000, true, false);
    sim.run(5000);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "latched at boot");

    sim.up_pressed = false;
    sim.run(300);
    sim.up_pressed = true;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "fresh press moves");
}

void test_both_switches_pressed_stops()
{
    Sim sim;
    sim.up_pressed = true;
    sim.run(1000);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "opening");

    sim.down_pressed = true;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "both pressed");

    // Releasing one leaves the other held: it is not a new press, no start.
    sim.up_pressed = false;
    sim.run(3000);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "remaining held switch is not a new press");

    // Mirror case: keep UP held, release DOWN.
    Sim mirror;
    mirror.up_pressed = true;
    mirror.down_pressed = true;
    mirror.run(1000);
    mirror.down_pressed = false;
    mirror.run(3000);
    assert_dir(BlindsDirection::STOPPED, mirror.dir(), "remaining held UP is not a new press");
    TEST_ASSERT_EQUAL_INT(0, mirror.relay_transitions);
}

void test_short_tap_is_not_lost_inside_blanking_window()
{
    // Press long enough to be accepted, then release while the relay-noise
    // blanking window is still open. The release must be deferred, not
    // dropped, or the motor would run until the 60 s cap.
    Sim sim;
    sim.up_pressed = true;
    sim.run(DEBOUNCE + 1);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "started");
    sim.run(40);
    sim.up_pressed = false;

    sim.run(BLANKING + DEBOUNCE + 5);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "tap stopped right after blanking");
}

void test_reversal_waits_dead_time_and_never_overlaps()
{
    Sim sim;
    sim.up_pressed = true;
    sim.run(2000);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "opening");

    // User flips a rocker from UP straight to DOWN.
    sim.up_pressed = false;
    sim.down_pressed = true;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped, waiting dead time");

    sim.run(DEAD_TIME);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "closing after dead time");
    // The per-tick invariant already checked the gap; check it explicitly too.
    TEST_ASSERT_TRUE(sim.up_off_at > 0);
}

void test_pending_reversal_is_cancelled_if_released_during_dead_time()
{
    Sim sim;
    sim.up_pressed = true;
    sim.run(2000);
    sim.up_pressed = false;
    sim.down_pressed = true;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "waiting dead time");

    sim.down_pressed = false;
    sim.run(5000);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "never started");
    TEST_ASSERT_FALSE(sim.blinds.relay_down());
}

void test_same_direction_restart_has_no_dead_time()
{
    Sim sim;
    sim.up_pressed = true;
    sim.run(1000);
    sim.up_pressed = false;
    sim.run(300);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped");
    sim.up_pressed = true;
    sim.run(DEBOUNCE + 2);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "jog again immediately");
}

// ---------------------------------------------------------------------------
// Noise rejection

void test_glitches_shorter_than_debounce_are_ignored()
{
    Sim sim;
    for (int i = 0; i < 200; i++)
    {
        sim.up_pressed = true;
        sim.run(DEBOUNCE - 10);
        sim.up_pressed = false;
        sim.run(1);
    }
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "glitch train rejected");
    TEST_ASSERT_EQUAL_INT(0, sim.relay_transitions);
}

void test_mains_hum_on_released_input_is_ignored()
{
    // 50 Hz coupling on long wires: the released input reads low 5 ms out of
    // every 20 ms.
    Sim sim;
    for (int cycle = 0; cycle < 1000; cycle++)
    {
        sim.down_pressed = true;
        sim.run(5);
        sim.down_pressed = false;
        sim.run(15);
    }
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "hum rejected");
    TEST_ASSERT_EQUAL_INT(0, sim.relay_transitions);
}

void test_false_release_glitches_while_held_do_not_stop()
{
    Sim sim;
    sim.up_pressed = true;
    sim.run(500);
    for (int i = 0; i < 100; i++)
    {
        sim.up_pressed = false;
        sim.run(DEBOUNCE - 10);
        sim.up_pressed = true;
        sim.run(20);
    }
    assert_dir(BlindsDirection::OPENING, sim.dir(), "still opening");
    TEST_ASSERT_EQUAL_INT(1, sim.relay_transitions);
}

void test_contact_bounce_on_press_and_release_gives_one_movement()
{
    Sim sim;
    for (int i = 0; i < 10; i++)
    {
        sim.up_pressed = (i % 2) == 0;
        sim.run(3);
    }
    sim.up_pressed = true;
    sim.run(2000);
    for (int i = 0; i < 10; i++)
    {
        sim.up_pressed = (i % 2) == 1;
        sim.run(3);
    }
    sim.up_pressed = false;
    sim.run(1000);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped");
    TEST_ASSERT_EQUAL_INT(2, sim.relay_transitions);
}

// Random noise on both inputs after every relay change, for a range of
// noise durations and seeds. A real 3 s hold must give exactly one movement,
// and the system must be quiet once the user lets go.
void test_random_relay_noise_does_not_cause_a_loop()
{
    const uint32_t durations[] = {0, 20, 50, 100, 150, 200, 300, 500, 1000};
    for (uint32_t d : durations)
    {
        for (uint32_t seed = 1; seed <= 25; seed++)
        {
            Sim sim;
            sim.noise = NoiseMode::RANDOM_AFTER_RELAY_CHANGE;
            sim.noise_ms = d;
            sim.rng = Rng(seed * 7919);

            sim.up_pressed = true;
            sim.run(3000);
            sim.up_pressed = false;
            sim.run(10000);

            char msg[96];
            snprintf(msg, sizeof(msg), "noise %lu ms, seed %lu", (unsigned long)d, (unsigned long)seed);
            assert_dir(BlindsDirection::STOPPED, sim.dir(), msg);
            TEST_ASSERT_EQUAL_INT_MESSAGE(1, sim.starts, msg);
            TEST_ASSERT_EQUAL_INT_MESSAGE(2, sim.relay_transitions, msg);
        }
    }
}

// Worst case, and the "loop" seen on site: switching the motor off makes the
// released input read as pressed for longer than blanking + debounce. Each
// stop looks like a new press, each start's noise ends, the input reads
// released again, and so on. The lockout must bound it and then go quiet.
void test_feedback_loop_is_bounded_by_lockout_and_settles()
{
    Sim sim;
    sim.noise = NoiseMode::STUCK_PRESSED_AFTER_RELAY_CHANGE;
    sim.noise_ms = 400;
    sim.noise_on_down = false; // one input only: the "both pressed" rule can't mask it

    // One real tap.
    sim.up_pressed = true;
    sim.run(200);
    sim.up_pressed = false;

    sim.run(60000);

    assert_dir(BlindsDirection::STOPPED, sim.dir(), "settled");
    TEST_ASSERT_FALSE(sim.blinds.relay_up());
    TEST_ASSERT_FALSE(sim.blinds.relay_down());
    // 8 allowed starts in the window, then lockout. Each start has a stop.
    TEST_ASSERT_LESS_OR_EQUAL_INT(2 * 8, sim.relay_transitions);

    // Lockout has cleared (inputs quiet and released for 2 s)...
    TEST_ASSERT_FALSE(sim.blinds.switch_lockout());

    // ...and with no relay activity there is no more noise: no movement for
    // a long time.
    const int transitions = sim.relay_transitions;
    sim.run(600000);
    TEST_ASSERT_EQUAL_INT(transitions, sim.relay_transitions);
}

void test_noise_shorter_than_blanking_plus_debounce_never_loops()
{
    for (uint32_t d = 0; d < BLANKING + DEBOUNCE; d += 10)
    {
        Sim sim;
        sim.noise = NoiseMode::STUCK_PRESSED_AFTER_RELAY_CHANGE;
        sim.noise_ms = d;
        sim.noise_on_down = false;

        sim.up_pressed = true;
        sim.run(2000);
        sim.up_pressed = false;
        sim.run(20000);

        char msg[64];
        snprintf(msg, sizeof(msg), "noise %lu ms", (unsigned long)d);
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, sim.relay_transitions, msg);
        TEST_ASSERT_FALSE_MESSAGE(sim.blinds.switch_lockout(), msg);
    }
}

// Same noise scenarios with a sparse, jittery loop (3..20 ms per tick), to
// show nothing depends on sampling every millisecond.
void test_noise_handling_with_jittery_loop_timing()
{
    for (uint32_t seed = 1; seed <= 20; seed++)
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "jitter seed %lu", (unsigned long)seed);

        Sim random_noise;
        random_noise.jitter_max_ms = 20;
        random_noise.jitter = Rng(seed);
        random_noise.noise = NoiseMode::RANDOM_AFTER_RELAY_CHANGE;
        random_noise.noise_ms = 500;
        random_noise.rng = Rng(seed * 31);
        random_noise.up_pressed = true;
        random_noise.run(3000);
        random_noise.up_pressed = false;
        random_noise.run(10000);
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, random_noise.relay_transitions, msg);

        Sim loop;
        loop.jitter_max_ms = 20;
        loop.jitter = Rng(seed);
        loop.noise = NoiseMode::STUCK_PRESSED_AFTER_RELAY_CHANGE;
        loop.noise_ms = 400;
        loop.noise_on_down = false;
        loop.up_pressed = true;
        loop.run(200);
        loop.up_pressed = false;
        loop.run(60000);
        assert_dir(BlindsDirection::STOPPED, loop.dir(), msg);
        TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(2 * 8, loop.relay_transitions, msg);
        TEST_ASSERT_FALSE_MESSAGE(loop.blinds.switch_lockout(), msg);
    }
}

void test_human_jogging_does_not_trigger_lockout()
{
    Sim sim;
    // 8 short taps in ~6 s: the most a person plausibly does.
    for (int i = 0; i < 8; i++)
    {
        sim.up_pressed = true;
        sim.run(300);
        sim.up_pressed = false;
        sim.run(450);
    }
    TEST_ASSERT_FALSE(sim.blinds.switch_lockout());
    TEST_ASSERT_EQUAL_INT(8, sim.starts);
}

void test_lockout_blocks_switch_starts_until_released_for_a_while()
{
    Sim sim;
    // 9 deliberate taps in ~6.3 s: one more start than the lockout allows.
    for (int i = 0; i < 9; i++)
    {
        sim.up_pressed = true;
        sim.run(300);
        sim.up_pressed = false;
        sim.run(400);
    }
    TEST_ASSERT_TRUE(sim.blinds.switch_lockout());
    TEST_ASSERT_EQUAL_INT(8, sim.starts);

    // Pressing during lockout does nothing, and holding keeps it locked.
    sim.up_pressed = true;
    sim.run(5000);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "locked out");
    TEST_ASSERT_TRUE(sim.blinds.switch_lockout());

    // Released, but not long enough.
    sim.up_pressed = false;
    sim.run(1500);
    TEST_ASSERT_TRUE(sim.blinds.switch_lockout());

    sim.run(1000);
    TEST_ASSERT_FALSE(sim.blinds.switch_lockout());

    sim.up_pressed = true;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "works again");
}

void test_remote_commands_work_during_lockout()
{
    Sim sim;
    // 9 deliberate taps in ~6.3 s: one more start than the lockout allows.
    for (int i = 0; i < 9; i++)
    {
        sim.up_pressed = true;
        sim.run(300);
        sim.up_pressed = false;
        sim.run(400);
    }
    TEST_ASSERT_TRUE(sim.blinds.switch_lockout());

    sim.up_pressed = true; // keep it locked
    sim.run(100);
    sim.command(-50);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "remote moves during lockout");
    sim.run(1000);
    sim.command(0);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "remote stop during lockout");
}

// ---------------------------------------------------------------------------
// Remote (MQTT) commands

void test_remote_partial_movements_use_interpolated_durations()
{
    Sim sim;
    sim.command(50);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "opening");
    sim.run(FULL_OPEN / 2 - 2);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "before 50%");
    sim.run(3);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "after 50%");

    sim.run(DEAD_TIME); // reversing: let the dead time pass first
    sim.command(-25);
    sim.run(18333 / 4 - 2);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "before 25%");
    sim.run(3);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "after 25%");
}

void test_remote_full_open_uses_full_open_command_time()
{
    Sim sim;
    sim.command(100);
    sim.run(23500 - 2);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "still opening");
    sim.run(3);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "done");

    TEST_ASSERT_EQUAL_UINT32(18333, sim.blinds.movement_duration(BlindsDirection::CLOSING, 100.0f));
}

void test_remote_command_while_moving_only_stops()
{
    Sim sim;
    sim.command(100);
    sim.run(1000);
    sim.command(-50);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped, not reversed");
    sim.run(5000);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stays stopped");
}

void test_remote_zero_stops_and_tiny_moves_are_ignored()
{
    Sim sim;
    sim.command(-100);
    sim.run(500);
    sim.command(0);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped");

    sim.run(1000);
    sim.command(1); // 166 ms < 250 ms minimum
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "ignored");
    sim.command(0);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "still stopped");
}

void test_remote_reverse_right_after_stop_waits_dead_time()
{
    Sim sim;
    sim.command(100);
    sim.run(1000);
    sim.command(0);
    sim.run(10);
    sim.command(-50);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "queued");
    sim.run(DEAD_TIME - 10 - 1);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "still in dead time");
    sim.run(1);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "closing exactly at dead time");

    // The timed duration counts from when the relay actually energized.
    sim.run(18333 / 2 - 1);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "full duration");
    sim.run(2);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped after full duration");
}

void test_remote_command_while_queued_cancels_it()
{
    Sim sim;
    sim.command(100);
    sim.run(1000);
    sim.command(0);
    sim.command(-50);
    sim.command(-50); // second command while queued: stop
    sim.run(2000);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "cancelled");
}

void test_switch_takes_over_remote_movement_in_same_direction()
{
    Sim sim;
    sim.command(25);
    sim.run(500);
    sim.up_pressed = true;
    sim.run(10000); // longer than the 25% duration
    assert_dir(BlindsDirection::OPENING, sim.dir(), "held switch keeps it going");
    sim.up_pressed = false;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "release stops");
    TEST_ASSERT_EQUAL_INT(1, sim.starts);
}

void test_switch_reverses_remote_movement_with_dead_time()
{
    Sim sim;
    sim.command(100);
    sim.run(2000);
    sim.down_pressed = true;
    sim.run(DEBOUNCE + DEAD_TIME + 5);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "switch wins");
}

void test_switch_release_does_not_stop_remote_movement()
{
    Sim sim;
    sim.up_pressed = true;
    sim.run(500);
    sim.command(0);
    sim.run(1000);
    sim.command(-50);
    sim.run(500);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "remote closing");
    sim.up_pressed = false; // unrelated switch released
    sim.run(500);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "still closing");
}

void test_remote_stop_while_switch_held_does_not_restart()
{
    Sim sim;
    sim.down_pressed = true;
    sim.run(1000);
    sim.command(0);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped");
    sim.run(10000);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "held switch does not restart");
}

// ---------------------------------------------------------------------------
// Time edge cases (devices run for months)

void test_millis_rollover_during_remote_movement()
{
    Sim sim(0xFFFFFFFFu - 1000);
    sim.command(50);
    sim.run(FULL_OPEN / 2 - 2);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "across rollover");
    sim.run(3);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped on time");
}

void test_millis_rollover_during_debounce_and_hold()
{
    Sim sim(0xFFFFFFFFu - 40);
    sim.up_pressed = true;
    sim.run(DEBOUNCE - 1);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "not yet accepted");
    sim.run(2);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "accepted across rollover");
    sim.run(5000);
    assert_dir(BlindsDirection::OPENING, sim.dir(), "not stopped by rollover");
    sim.up_pressed = false;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "released");
}

void test_stale_timestamps_do_not_wake_up_after_rollover()
{
    // Stop a movement, then let exactly 2^32 ms pass in big steps: the old
    // stop timestamp equals "now" again. The dead time and blanking window
    // must not come back.
    Sim sim;
    sim.command(100);
    sim.run(1000);
    sim.command(0);
    const uint32_t stopped_at = sim.now;
    sim.run(DEAD_TIME + BLANKING);

    const uint32_t step = 1u << 20;
    for (uint32_t i = 0; i < 4095; i++)
    {
        sim.now += step;
        sim.blinds.tick(sim.now, false, false);
    }
    sim.now = stopped_at + 10; // 2^32 + 10 ms after the stop
    sim.blinds.tick(sim.now, false, false);
    // The harness's own "UP went off at" timestamp is just as stale; drop it
    // so the per-tick dead-time check doesn't misfire.
    sim.up_ever_off = false;
    sim.command(-50);
    assert_dir(BlindsDirection::CLOSING, sim.dir(), "no phantom dead time");
}

void test_long_loop_stall_still_stops()
{
    // EspMQTTClient can block loop() for seconds while reconnecting.
    Sim sim;
    sim.command(25);
    sim.run(100);
    sim.now += 30000;
    sim.blinds.tick(sim.now, false, false);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "stopped at the first tick after the stall");

    sim.up_pressed = true;
    sim.run(500);
    sim.now += 10000; // released during a stall
    sim.up_pressed = false;
    sim.run(DEBOUNCE + 5);
    assert_dir(BlindsDirection::STOPPED, sim.dir(), "release seen right after the stall");
}

// ---------------------------------------------------------------------------
// Parsing and settings

void test_parse_movement()
{
    float v = 0;
    TEST_ASSERT_TRUE(BlindsController::parse_movement("50", &v));
    TEST_ASSERT_EQUAL_FLOAT(50.0f, v);
    TEST_ASSERT_TRUE(BlindsController::parse_movement(" -12.5 \n", &v));
    TEST_ASSERT_EQUAL_FLOAT(-12.5f, v);
    TEST_ASSERT_TRUE(BlindsController::parse_movement("250", &v));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, v);
    TEST_ASSERT_TRUE(BlindsController::parse_movement("-1000", &v));
    TEST_ASSERT_EQUAL_FLOAT(-100.0f, v);
    TEST_ASSERT_TRUE(BlindsController::parse_movement("0", &v));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, v);

    v = 7;
    TEST_ASSERT_FALSE(BlindsController::parse_movement("", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_movement("   ", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_movement("up", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_movement("50%", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_movement("nan", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_movement("inf", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_movement("1e999", &v));
    TEST_ASSERT_EQUAL_FLOAT(7.0f, v);
}

void test_parse_timing_and_setters()
{
    uint32_t v = 0;
    TEST_ASSERT_TRUE(BlindsController::parse_timing("20000", &v));
    TEST_ASSERT_EQUAL_UINT32(20000, v);
    TEST_ASSERT_TRUE(BlindsController::parse_timing(" 1000 ", &v));
    TEST_ASSERT_TRUE(BlindsController::parse_timing("60000", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_timing("999", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_timing("60001", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_timing("12.5", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_timing("abc", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_timing("", &v));
    TEST_ASSERT_FALSE(BlindsController::parse_timing("99999999999999999999", &v));

    BlindsController c;
    TEST_ASSERT_TRUE(c.set_full_open_time(20000));
    TEST_ASSERT_EQUAL_UINT32(10000, c.movement_duration(BlindsDirection::OPENING, 50.0f));
    TEST_ASSERT_FALSE(c.set_full_close_time(0));
    TEST_ASSERT_EQUAL_UINT32(18333, c.full_close_time());
    TEST_ASSERT_TRUE(c.set_full_open_command_time(30000));
    TEST_ASSERT_EQUAL_UINT32(30000, c.movement_duration(BlindsDirection::OPENING, 100.0f));
}

int main(int, char **)
{
    UNITY_BEGIN();

    RUN_TEST(test_switch_press_moves_until_release_not_a_fixed_time);
    RUN_TEST(test_down_switch_moves_until_release);
    RUN_TEST(test_press_is_accepted_only_after_debounce_time);
    RUN_TEST(test_held_switch_is_capped_by_safety_timeout);
    RUN_TEST(test_switch_latched_at_boot_does_not_move);
    RUN_TEST(test_both_switches_pressed_stops);
    RUN_TEST(test_short_tap_is_not_lost_inside_blanking_window);
    RUN_TEST(test_reversal_waits_dead_time_and_never_overlaps);
    RUN_TEST(test_pending_reversal_is_cancelled_if_released_during_dead_time);
    RUN_TEST(test_same_direction_restart_has_no_dead_time);

    RUN_TEST(test_glitches_shorter_than_debounce_are_ignored);
    RUN_TEST(test_mains_hum_on_released_input_is_ignored);
    RUN_TEST(test_false_release_glitches_while_held_do_not_stop);
    RUN_TEST(test_contact_bounce_on_press_and_release_gives_one_movement);
    RUN_TEST(test_random_relay_noise_does_not_cause_a_loop);
    RUN_TEST(test_feedback_loop_is_bounded_by_lockout_and_settles);
    RUN_TEST(test_noise_shorter_than_blanking_plus_debounce_never_loops);
    RUN_TEST(test_noise_handling_with_jittery_loop_timing);
    RUN_TEST(test_human_jogging_does_not_trigger_lockout);
    RUN_TEST(test_lockout_blocks_switch_starts_until_released_for_a_while);
    RUN_TEST(test_remote_commands_work_during_lockout);

    RUN_TEST(test_remote_partial_movements_use_interpolated_durations);
    RUN_TEST(test_remote_full_open_uses_full_open_command_time);
    RUN_TEST(test_remote_command_while_moving_only_stops);
    RUN_TEST(test_remote_zero_stops_and_tiny_moves_are_ignored);
    RUN_TEST(test_remote_reverse_right_after_stop_waits_dead_time);
    RUN_TEST(test_remote_command_while_queued_cancels_it);
    RUN_TEST(test_switch_takes_over_remote_movement_in_same_direction);
    RUN_TEST(test_switch_reverses_remote_movement_with_dead_time);
    RUN_TEST(test_switch_release_does_not_stop_remote_movement);
    RUN_TEST(test_remote_stop_while_switch_held_does_not_restart);

    RUN_TEST(test_millis_rollover_during_remote_movement);
    RUN_TEST(test_millis_rollover_during_debounce_and_hold);
    RUN_TEST(test_stale_timestamps_do_not_wake_up_after_rollover);
    RUN_TEST(test_long_loop_stall_still_stops);

    RUN_TEST(test_parse_movement);
    RUN_TEST(test_parse_timing_and_setters);

    return UNITY_END();
}
