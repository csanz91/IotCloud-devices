#include <Arduino.h>
#include <iotcloud.h>
#include <notifier.h>

Device my_device = Device("v1.0.5", "SONOFF_RF");

NOTIFIER *notifier = new NOTIFIER("001_N", "Door", "{}");

void setup(void)
{
    Serial.begin(19200);

    my_device.add_sensor(notifier);
    iotcloud_setup(&my_device);
}

const int reservedButtonMemory = 24;
const char endMarker = 0x55;
const char startMarker = 0xAA;
const char actionForwardCode = 0xA4;
unsigned long _last_debounce_time = 0;
const int long DEBOUNCE_TIME = 3000;
char receivedChars[reservedButtonMemory];
char idx = 0;

void recv_data()
{

    while (Serial.available() > 0)
    {
        int rc = Serial.read();

        // If we have not reached the end of the string
        if (rc != endMarker)
        {
            // Save the char
            receivedChars[idx] = rc;

            idx++;
            // Invalid input. Abort.
            if (idx >= reservedButtonMemory)
            {
                idx = reservedButtonMemory - 1;
            }
        }
        // We have reached the end of the string
        else
        {
            idx = 0;

            const unsigned long now = millis();

            if (receivedChars[0] == startMarker && receivedChars[1] == actionForwardCode && (now - _last_debounce_time > DEBOUNCE_TIME))
            {
                digitalWrite(13, LOW);
                uint16_t tsyn = (receivedChars[2] << 8) | (receivedChars[3]);
                uint16_t tlow = (receivedChars[4] << 8) | (receivedChars[5]);
                uint16_t thigh = (receivedChars[6] << 8) | (receivedChars[7]);
                uint32_t dataValue = ((uint32_t)receivedChars[8] << 16) | ((uint32_t)receivedChars[9] << 8) | (uint32_t)receivedChars[10];

                char formattedString[25];
                sprintf(formattedString, "Puerta de casa abierta", dataValue);
                Serial.println(formattedString);
                if (dataValue == 1906529)
                {
                    notifier->send_notification(formattedString);
                }
            }
            _last_debounce_time = millis();
            return;
        }
    }
}

void loop(void)
{
    iotcloud_loop();
    recv_data();
}