#include <Arduino.h>
#include <iotcloud.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266HTTPClient.h>

#include "temp_hum_sensor.h"
#include <wifi_rssid.h>
#include <si7021_am2301.h>

#define SSD1306_NO_SPLASH
#define PIN D1
#define SDA D6
#define SCL D5
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Device my_device = Device("v1.0", "SI7021_OLED");

SI7021_AM2301 sensor = SI7021_AM2301(PIN, SI7021_AM2301::SensorModel::SI7021);

const char *serverIP = "192.168.0.155"; // Replace with the server's IP address
const int serverPort = 8000;            // Replace with the server's port
int x, minX;

String message = "";

void fetchStringContents()
{
    WiFiClient client;

    if (client.connect(serverIP, serverPort))
    {
        client.print(String("GET /") + " HTTP/1.1\r\n" +
                     "Host: " + serverIP + "\r\n" +
                     "Connection: close\r\n" +
                     "\r\n");

        while (client.connected() || client.available())
        {
            if (client.available())
            {
                message = client.readStringUntil('\n');
                minX = -18 * message.length(); // 18 = 6 pixels/character * text size 3
            }
        }
        client.stop();
    }
    else
    {
        Serial.println("connection failed!]");
        client.stop();
    }
}

void setup(void)
{
    Serial.begin(115200);

    Wire.begin(SDA, SCL);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    // Clear the buffer
    display.clearDisplay();

    display.setTextSize(3); // Draw 3X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 5);

    display.setTextWrap(false);
    x = display.width();

    display.display();

    my_device.add_sensor(new IotCloud_T_H("001_T", "Temperature", "{\"engUnits\": \"\xB0"
                                                                  "C\", \"stringFormat\": \"#0.0\"}",
                                          &sensor.temperature));
    my_device.add_sensor(new IotCloud_T_H("001_H", "Humidity", "{\"engUnits\": \"%\", \"stringFormat\": \"#0\"}", &sensor.humidity));
    iotcloud_setup(&my_device);
}

unsigned long last_reading = 0;
unsigned long last_reading_oled = 0;

void loop(void)
{
    unsigned long ellapsed_since_last_read = millis() - last_reading;
    if (ellapsed_since_last_read > 5000)
    {
        sensor.read_values();
        // Fetch the string contents
        last_reading = millis();
    }
    unsigned long ellapsed_since_last_read_oled = millis() - last_reading_oled;
    if (ellapsed_since_last_read_oled > 60000)
    {
        // Fetch the string contents
        fetchStringContents();
        last_reading_oled = millis();
    }
    iotcloud_loop();

    display.clearDisplay();
    display.setCursor(x, 5);
    display.print(message);
    display.display();

    if (--x < minX)
        x = display.width();
}