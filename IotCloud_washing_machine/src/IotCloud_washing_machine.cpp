#include <Arduino.h>
#include <iotcloud.h>

#include <notifier.h>
#include <servo_button.h>

// Owned by the IotCloud runtime. Checking it here prevents notifications from
// being discarded before the notifier has been initialized.
extern EspMQTTClient *mqtt_client;

namespace
{
// Hardware configuration
constexpr uint8_t SERVO_PIN = D2;
constexpr uint8_t LDR_PIN = A0;

// Servo configuration
constexpr uint32_t ACTIVATION_TIME_MS = 1000;
constexpr uint8_t ACTIVATION_ANGLE = 85;
constexpr uint8_t RESTING_ANGLE = 0;

// LED state detection configuration
constexpr int LDR_STABILITY_RANGE = 50;          // Analog points (0-1023)
constexpr uint32_t MEASUREMENT_INTERVAL_MS = 100;
constexpr uint32_t EXPECTED_BLINK_INTERVAL_MS = 500;
constexpr uint16_t BLINK_INTERVAL_MARGIN_SAMPLES = 1;
constexpr uint32_t LED_ON_DURATION_MS = 20000;
constexpr int LED_ON_DELTA = 250;                // Difference from the darkest reading
constexpr uint16_t DONE_BLINK_TRANSITIONS = 10;  // Ten transitions are five full blinks

static_assert(EXPECTED_BLINK_INTERVAL_MS % MEASUREMENT_INTERVAL_MS == 0,
              "Blink interval must be a multiple of the measurement interval");
static_assert(LED_ON_DURATION_MS % MEASUREMENT_INTERVAL_MS == 0,
              "LED-on duration must be a multiple of the measurement interval");

constexpr uint16_t EXPECTED_BLINK_SAMPLES =
    EXPECTED_BLINK_INTERVAL_MS / MEASUREMENT_INTERVAL_MS;
constexpr uint16_t LED_ON_SAMPLES =
    LED_ON_DURATION_MS / MEASUREMENT_INTERVAL_MS;

enum class PendingNotification : uint8_t
{
    NONE,
    RUNNING,
    DONE,
};

Device device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
NOTIFIER notifier("002_N", "Washing Machine", "{}");
constexpr char RUNNING_NOTIFICATION_PAYLOAD[] =
    "iotcloud-notification-v1:{\"title\":\"Lavadora en marcha\","
    "\"body\":\"La lavadora ha comenzado un ciclo\",\"critical\":true}";
constexpr char DONE_NOTIFICATION_PAYLOAD[] =
    "iotcloud-notification-v1:{\"title\":\"Lavado terminado\","
    "\"body\":\"El ciclo de lavado ha terminado\",\"critical\":true}";

uint32_t last_measurement_ms = 0;
int previous_ldr_value = 0;
int darkest_ldr_value = 0;
uint16_t stable_sample_count = 0;
uint16_t blink_transition_count = 0;
bool has_previous_measurement = false;

PendingNotification pending_notification = PendingNotification::NONE;
bool running_notification_detected = false;
bool done_notification_detected = false;

bool is_measurement_stable(int ldr_value)
{
    if (!has_previous_measurement)
    {
        previous_ldr_value = ldr_value;
        has_previous_measurement = true;
        return false;
    }

    const int difference = ldr_value - previous_ldr_value;
    previous_ldr_value = ldr_value;

    return difference >= -(LDR_STABILITY_RANGE / 2) &&
           difference <= LDR_STABILITY_RANGE / 2;
}

bool record_blink_transition(bool is_stable)
{
    if (is_stable)
    {
        return false;
    }

    const uint16_t minimum_samples =
        EXPECTED_BLINK_SAMPLES - BLINK_INTERVAL_MARGIN_SAMPLES;
    const uint16_t maximum_samples =
        EXPECTED_BLINK_SAMPLES + BLINK_INTERVAL_MARGIN_SAMPLES;

    if (stable_sample_count < minimum_samples || stable_sample_count > maximum_samples)
    {
        return false;
    }

    if (blink_transition_count < DONE_BLINK_TRANSITIONS)
    {
        ++blink_transition_count;
    }

    // The transition sample is the first sample of the new LED level.
    stable_sample_count = 1;
    return true;
}

bool is_led_on()
{
    return darkest_ldr_value - previous_ldr_value > LED_ON_DELTA &&
           stable_sample_count >= LED_ON_SAMPLES;
}

void update_machine_state()
{
    if (!done_notification_detected &&
        blink_transition_count >= DONE_BLINK_TRANSITIONS)
    {
        done_notification_detected = true;
        pending_notification = PendingNotification::DONE;
        Serial.println(F("Washing machine done"));
        return;
    }

    // The device is power-cycled for every wash, so each event must be emitted
    // at most once per boot. In particular, never report "on" after "done".
    if (!done_notification_detected &&
        !running_notification_detected &&
        is_led_on())
    {
        running_notification_detected = true;
        pending_notification = PendingNotification::RUNNING;
        Serial.println(F("Washing machine on"));
    }
}

void sample_ldr()
{
    const int ldr_value = analogRead(LDR_PIN);
    darkest_ldr_value = max(ldr_value, darkest_ldr_value);

    const bool is_stable = is_measurement_stable(ldr_value);
    if (is_stable && stable_sample_count < LED_ON_SAMPLES)
    {
        ++stable_sample_count;
    }

    const bool is_blink_transition = record_blink_transition(is_stable);
    if (!is_stable && !is_blink_transition)
    {
        blink_transition_count = 0;
        stable_sample_count = 0;
    }

    update_machine_state();
}

void send_pending_notification()
{
    if (pending_notification == PendingNotification::NONE ||
        mqtt_client == nullptr || !mqtt_client->isConnected())
    {
        return;
    }

    if (pending_notification == PendingNotification::DONE)
    {
        notifier.send_notification(DONE_NOTIFICATION_PAYLOAD);
    }
    else
    {
        notifier.send_notification(RUNNING_NOTIFICATION_PAYLOAD);
    }

    pending_notification = PendingNotification::NONE;
}
} // namespace

void setup()
{
    Serial.begin(115200);
    pinMode(LDR_PIN, INPUT);

    static SERVO_BUTTON start_button(
        "001_W",
        "Washing Machine",
        "{}",
        ACTIVATION_TIME_MS,
        ACTIVATION_ANGLE,
        RESTING_ANGLE,
        SERVO_PIN);

    device.add_sensor(&start_button);
    device.add_sensor(&notifier);
    iotcloud_setup(&device);
}

void loop()
{
    iotcloud_loop();
    send_pending_notification();

    const uint32_t now = millis();
    if (now - last_measurement_ms >= MEASUREMENT_INTERVAL_MS)
    {
        last_measurement_ms = now;
        sample_ldr();
    }
}
