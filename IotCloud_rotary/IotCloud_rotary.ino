#include <Arduino.h>
#include <iotcloud.h>
#include <sensors/analog.h>
#include "AiEsp32RotaryEncoder.h"

#define ROTARY_ENCODER_A_PIN 5
#define ROTARY_ENCODER_B_PIN 6
#define ROTARY_ENCODER_BUTTON_PIN 7

#define ROTARY_ENCODER_VCC_PIN -1

#define ROTARY_ENCODER_STEPS 4

//instead of changing here, rather change numbers above
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

//paramaters for button
unsigned long shortPressAfterMiliseconds = 150;  //how long short press shoud be. Do not set too low to avoid bouncing (false press events).
unsigned long longPressAfterMiliseconds = 1000;  //how long čong press shoud be.

Device my_device = Device("v1.0.4", "Rotary");
AnalogSensor *analog_sensor = new AnalogSensor("001_E", "Rotary", 300000, 0, false, "analog_subsensor", "{}", 0.0, false);


unsigned long last_rotary_check = 0;
long last_rotary_value = 0;
long accumulated_diff = 0;
unsigned long last_report_time = 0;

void on_button_short_click() {
  analog_sensor->set_value(255);
  accumulated_diff = 0;
}

void on_button_long_click() {
  analog_sensor->set_value(255);
  accumulated_diff = 0;
}

void handle_rotary_button() {
  static unsigned long lastTimeButtonDown = 0;
  static bool wasButtonDown = false;

  bool isEncoderButtonDown = rotaryEncoder.isEncoderButtonDown();
  //isEncoderButtonDown = !isEncoderButtonDown; //uncomment this line if your button is reversed

  if (isEncoderButtonDown) {
    if (!wasButtonDown) {
      //start measuring
      lastTimeButtonDown = millis();
    }
    //else we wait since button is still down
    wasButtonDown = true;
    return;
  }

  //button is up
  if (wasButtonDown) {
    //click happened, lets see if it was short click, long click or just too short
    if (millis() - lastTimeButtonDown >= longPressAfterMiliseconds) {
      on_button_long_click();
    } else if (millis() - lastTimeButtonDown >= shortPressAfterMiliseconds) {
      on_button_short_click();
    }
  }
  wasButtonDown = false;
}

void rotary_loop() {
  unsigned long current_time = millis();
  if (current_time - last_rotary_check < 1) {
    return;
  }
  last_rotary_check = current_time;

  if (rotaryEncoder.encoderChanged()) {
    long current_val = rotaryEncoder.readEncoder();
    long diff = last_rotary_value - current_val;

    // Always use the shortest path
    if (diff > 50) {  // if difference is more than 50, it's shorter to go the other way
      diff = diff - 101;
    } else if (diff < -50) {  // same for negative differences
      diff = 101 + diff;
    }

    last_rotary_value = current_val;
    accumulated_diff += diff;  // Accumulate the difference instead of reporting directly
    // Serial.println(accumulated_diff);
  }

  // Only report accumulated value every 250ms
  if (current_time - last_report_time >= 150) {
    if (accumulated_diff != 0) {  // Only report if there's a change
      analog_sensor->set_value(float(accumulated_diff));
      accumulated_diff = 0;  // Reset accumulator
    }
    last_report_time = current_time;
  }
  handle_rotary_button();
}

void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}


void setup(void) {
  Serial.begin(115200);

  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  bool circleValues = true;
  rotaryEncoder.setBoundaries(0, 100, circleValues);  //minValue, maxValue, circleValues true|false (when max go to min and vice versa)

  rotaryEncoder.setAcceleration(50);  //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration

  my_device.add_sensor(analog_sensor);
  iotcloud_setup(&my_device);
}

void loop(void) {
  iotcloud_loop();
  rotary_loop();
}