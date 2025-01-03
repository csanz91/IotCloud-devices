#include <Arduino.h>
#include <iotcloud.h>

#include <servo_button.h>
#include <notifier.h>

// HW configuration
#define PIN D2
#define LDR_PIN A0

// Servo configuration
const long unsigned int ACTIVATION_TIME = 1000;
const long unsigned int ACTIVATION_ANGLE = 90;
const long unsigned int RESTING_ANGLE = 0;

// LED states detection configuration
const int stability_threshold = 50;       // Analog points 0-1023
const int measurement_interval = 100;     // ms
const int expected_blink_frequency = 500; // ms
const int num_stable_measurements_threshold_margin = 1;
const int led_on_threshold = 20000;           // Milliseconds with the LED light stable to consider it ON
const int led_on_measurement_threshold = 250; // Analog points (0-1023) from the darkest measurement to consider the LED ON
const int num_blink_cycles_threshold = 10;    // Number of blinks to detect a done state

Device my_device = Device("v1.0", "WASHING_MACHINE");

NOTIFIER *notifier = new NOTIFIER("002_N", "Wachine Machine", "{}");

void setup(void)
{
    Serial.begin(115200);

    my_device.add_sensor(new SERVO_BUTTON("001_W", "Wachine Machine", "{}", ACTIVATION_TIME, ACTIVATION_ANGLE, RESTING_ANGLE, PIN));
    my_device.add_sensor(notifier);
    iotcloud_setup(&my_device);

    pinMode(LDR_PIN, INPUT);
}

long unsigned int last_measurement = 0;
int mem_ldr_value = 0;
int num_stable_measurements = 0;
int num_blink_cycles = 0;
int num_stable_measurements_blinking_threshold = expected_blink_frequency / measurement_interval;
int num_stable_measurements_on_threshold = led_on_threshold / measurement_interval;
int max_ldr_value = 0;

bool done_notification_sent = false;
bool on_notification_sent = false;

bool is_measurement_stable(int ldr_value)
{
    const bool is_stable = ldr_value >= mem_ldr_value - stability_threshold / 2 &&
                           ldr_value <= mem_ldr_value + stability_threshold / 2;
    mem_ldr_value = ldr_value;
    return is_stable;
}

bool is_blinking(bool is_stable)
{
    // Transition from blinking cycle
    if (!is_stable &&
        num_stable_measurements >= num_stable_measurements_blinking_threshold - num_stable_measurements_threshold_margin &&
        num_stable_measurements <= num_stable_measurements_blinking_threshold + num_stable_measurements_threshold_margin)
    {
        num_blink_cycles++;
        num_stable_measurements = 1;
        return true;
    }

    return false;
}

bool is_done()
{
    return num_blink_cycles >= num_blink_cycles_threshold;
}

bool is_on()
{
    return (max_ldr_value - led_on_measurement_threshold) > mem_ldr_value && num_stable_measurements >= num_stable_measurements_on_threshold;
}

void loop(void)
{
    if (millis() - last_measurement > measurement_interval)
    {
        last_measurement = millis();
        int ldr_value = analogRead(LDR_PIN);
        max_ldr_value = max(ldr_value, max_ldr_value);

        // Check if the measurement is stable
        const bool is_stable = is_measurement_stable(ldr_value);
        if (is_stable)
            num_stable_measurements++;

        // Check if the measurement is blinking
        const bool blinking = is_blinking(is_stable);
        if (!is_stable && !blinking)
        {
            num_blink_cycles = 0;
            num_stable_measurements = 0;
        }

        // Check if the machine is done and send notification
        if (is_done() && !done_notification_sent)
        {
            notifier->send_notification("Washing machine done");
            Serial.println("Washing machine done");
            done_notification_sent = true;
        }

        // Check if the machine is on and send notification
        if (is_on() && !on_notification_sent)
        {
            notifier->send_notification("Washing machine on");
            Serial.println("Washing machine on");
            on_notification_sent = true;
        }

        Serial.println("LDR: " + String(ldr_value));
        Serial.println("Num stable measurements: " + String(num_stable_measurements));
        Serial.println("Num blink cycles: " + String(num_blink_cycles));
    }
    iotcloud_loop();
}