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

Device my_device = Device(IOTCLOUD_FW_VERSION, IOTCLOUD_MODEL_NUM);
ThermostatSubsensor *subsensor = nullptr;

// Display state
volatile float displayTemp = NAN;
volatile float displayHumidity = NAN;
unsigned long lastDisplayUpdate = 0;
int displayScreen = 0;  // 0 = temperature, 1 = humidity, 2 = custom text
bool displayEnabled = true;

// Scroll state for long text
int scrollOffset = 0;
unsigned long lastScrollUpdate = 0;
const int SCROLL_SPEED_MS = 15;  // Scroll every 15ms for smooth animation

void applyDisplayBrightness(float brightness)
{
    if (isnan(brightness) || isinf(brightness))
    {
        return;
    }

    if (brightness < 0.0f)
        brightness = 0.0f;
    if (brightness > 1.0f)
        brightness = 1.0f;

    if (brightness <= 0.0f)
    {
        displayEnabled = false;
        display.ssd1306_command(SSD1306_DISPLAYOFF);
        return;
    }

    displayEnabled = true;
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(static_cast<uint8_t>(brightness * 255));

    lastDisplayUpdate = 0;
}

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

// Returns true if text needs scrolling (is wider than screen)
bool drawText(const char *text, int scrollPos)
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);  // Disable auto line-wrap - we want single-line horizontal scrolling
    display.setTextSize(4);  // Largest font for far-away readability (same as temp/humidity)
    
    // Get text bounds
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    
    // Vertically center the text
    int y = (SCREEN_HEIGHT - h) / 2 - y1;
    bool needsScroll = (w > SCREEN_WIDTH);
    
    if (needsScroll) {
        // Scrolling mode: text scrolls from right to left
        int x = SCREEN_WIDTH - scrollPos;  // Start from right edge, scroll left
        
        display.setCursor(x, y);
        display.print(text);
    } else {
        // Static mode: center the text horizontally
        int x = (SCREEN_WIDTH - w) / 2;
        display.setCursor(x, y);
        display.print(text);
    }
    
    display.display();
    return needsScroll;
}

void updateDisplay()
{
    if (!displayEnabled)
        return;

    unsigned long now = millis();
    static int cycleCount = 0;
    static bool isScrolling = false;
    static int textWidth = 0;
    
    // Handle custom text screen with scrolling
    if (displayScreen == 2 && subsensor && subsensor->has_active_custom_text()) {
        static int scrollCycleCount = 0;
        
        // For scrolling, update more frequently
        if (now - lastScrollUpdate >= SCROLL_SPEED_MS) {
            lastScrollUpdate = now;
            
            const char *text = subsensor->get_custom_text();
            isScrolling = drawText(text, scrollOffset);
            
            if (isScrolling) {
                // Calculate text width for wrap-around
                display.setTextSize(4);  // Must match drawText font size
                int16_t x1, y1;
                uint16_t w, h;
                display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
                textWidth = w;
                
                scrollOffset += 1;  // Scroll 1 pixel per step for smooth motion
                
                // Check if scroll cycle completed (text fully scrolled off left side)
                if (scrollOffset > textWidth + SCREEN_WIDTH) {
                    scrollOffset = 0;
                    scrollCycleCount++;
                }
            } else {
                // Static text (fits on screen) - count as 2 cycles immediately
                scrollCycleCount = 2;
            }
        }
        
        // Transition only after 2 scroll cycles completed
        if (scrollCycleCount >= 2) {
            displayScreen = 0;
            cycleCount = 0;
            scrollOffset = 0;
            scrollCycleCount = 0;
        }
        
        // Check TTL expiration
        if (!subsensor->has_active_custom_text()) {
            displayScreen = 0;
            cycleCount = 0;
            scrollOffset = 0;
            scrollCycleCount = 0;
        }
        return;
    }
    
    // Normal display update cadence for temp/humidity
    if (now - lastDisplayUpdate < 2000)
        return;
    
    lastDisplayUpdate = now;
    
    switch (displayScreen)
    {
    case 0:  // Temperature (show for 4 cycles = 8 seconds)
        drawValue(displayTemp, "C");
        break;
    case 1:  // Humidity (show for 2 cycles = 4 seconds)
        drawValue(displayHumidity, "%");
        break;
    case 2:  // Custom text - handled above, but fallback
        if (subsensor && subsensor->has_active_custom_text()) {
            drawText(subsensor->get_custom_text(), 0);
        }
        break;
    }
    
    // Cycle screens: temp -> humidity -> custom text (if active) -> temp
    cycleCount++;
    
    if (displayScreen == 0 && cycleCount >= 4)
    {
        displayScreen = 1;
        cycleCount = 0;
    }
    else if (displayScreen == 1 && cycleCount >= 2)
    {
        // After humidity, show custom text if active, otherwise go to temp
        if (subsensor && subsensor->has_active_custom_text()) {
            displayScreen = 2;
            scrollOffset = 0;  // Reset scroll position
            lastScrollUpdate = now;
        } else {
            displayScreen = 0;
        }
        cycleCount = 0;
    }
}

void onDisplayUpdate(float temperature, float humidity)
{
    displayTemp = temperature;
    displayHumidity = humidity;
}

void onDisplayBrightnessUpdate(float brightness)
{
    applyDisplayBrightness(brightness);
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
        30000               // report period (30 seconds)
    );
    
    subsensor->set_display_callback(onDisplayUpdate);
    subsensor->set_display_brightness_callback(onDisplayBrightnessUpdate);
    my_device.add_sensor(subsensor);
    
    iotcloud_setup(&my_device);
}

void loop()
{
    iotcloud_loop();
    updateDisplay();
}