/**
 * IotCloud Thermostat Subsensor
 * 
 * A remote temperature/humidity sensor that publishes readings to a main thermostat's
 * MQTT topic (impersonation via subsensorId). Features:
 * - SI7021 temp/humidity sensor reading
 * - OLED display (SSD1306 128x32) showing temperature and humidity
 * - Built-in spike filtering to reject anomalous readings
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <iotcloud.h>
#include <device_data.h>
#include <thermostat_subsensor.h>

// Pin definitions
#define SENSOR_PIN D1   // SI7021 data pin (GPIO5)
#define I2C_SDA_PIN D6  // OLED SDA (GPIO12)
#define I2C_SCL_PIN D5  // OLED SCL (GPIO14)

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1  // No reset pin
#define OLED_I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Device my_device = Device("v1.0", "THERMOSTAT_SUBSENSOR");
ThermostatSubsensor *subsensor = nullptr;

// Display state
volatile float displayTemp = NAN;
volatile float displayHumidity = NAN;
unsigned long lastDisplayUpdate = 0;
int displayScreen = 0;  // 0 = temperature, 1 = humidity

void drawValue(float value, const char *unit)
{
    display.clearDisplay();
    display.setTextSize(4);
    display.setTextColor(SSD1306_WHITE);
    
    char buffer[16];
    if (isnan(value))
    {
        strcpy(buffer, "---");
        strcat(buffer, unit);
    }
    else
    {
        dtostrf(value, 1, 1, buffer);
        strcat(buffer, unit);
    }

    // Center the text
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(buffer, 0, 0, &x1, &y1, &w, &h);
    int x = (SCREEN_WIDTH - w) / 2;
    int y = (SCREEN_HEIGHT - h) / 2;
    
    // Adjust y because getTextBounds y1 is usually the top-left offset
    // For the standard font, y1 is usually 0 or slightly negative
    display.setCursor(x, y - y1);
    display.print(buffer);
    
    display.display();
}

void updateDisplay()
{
    unsigned long now = millis();
    if (now - lastDisplayUpdate < 2000)  // Update every 2 seconds
        return;
    
    lastDisplayUpdate = now;
    
    switch (displayScreen)
    {
    case 0:  // Temperature (show for 4 cycles = 8 seconds)
        drawValue(displayTemp, "C");  // C
        break;
    case 1:  // Humidity (show for 2 cycles = 4 seconds)
        drawValue(displayHumidity, "%");
        break;
    }
    
    // Cycle screens: show temp longer than humidity (mimicking original behavior)
    static int cycleCount = 0;
    cycleCount++;
    if (displayScreen == 0 && cycleCount >= 4)
    {
        displayScreen = 1;
        cycleCount = 0;
    }
    else if (displayScreen == 1 && cycleCount >= 2)
    {
        displayScreen = 0;
        cycleCount = 0;
    }
}

void onDisplayUpdate(float temperature, float humidity)
{
    displayTemp = temperature;
    displayHumidity = humidity;
}

void setup()
{
    Serial.begin(115200);
    Serial.println(F("\nIotCloud Thermostat Subsensor"));
    
    // Initialize I2C for OLED
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Initialize OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
    }
    else
    {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(20, 12);
        display.print("Starting...");
        display.display();
    }
    
    subsensor = new ThermostatSubsensor(
        "001_T",           // sensor_id (internal)
        "Subsensor",       // sensor_name
        SENSOR_PIN,        // SI7021 data pin
        SI7021_AM2301::SensorModel::SI7021,  // sensor model
        30000,              // report period (30 seconds)
        0.1                // 10% filter rate for temperature (reject 17.5->19.0 spikes)
    );
    
    subsensor->set_display_callback(onDisplayUpdate);
    my_device.add_sensor(subsensor);
    
    iotcloud_setup(&my_device);
}

void loop()
{
    iotcloud_loop();
    updateDisplay();
}