#pragma once

// ---------------------------------------------------------------------------
// Hardware
// ---------------------------------------------------------------------------
// BH1750 / GY-302 I2C address: ADDR pin low (or floating on GY-302) -> 0x23,
// ADDR pin high -> 0x5C.
#define BH1750_ADDR 0x23

// NodeMCU default I2C pins: D2 = GPIO4 = SDA, D1 = GPIO5 = SCL.
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

// On-board LED (GPIO2 / D4, active low). Blinks while recording. Set
// STATUS_LED_ENABLED to 0 if the board sits where the sensor can see it.
#define STATUS_LED_ENABLED 1
#define STATUS_LED_PIN 2
#define STATUS_LED_ON LOW
#define STATUS_LED_OFF HIGH

// BH1750 datasheet conversion factor (typ. 1.2, spread 0.96..1.44).
// It only scales absolute lux; left/right and old/new comparisons made with
// the same sensor do not depend on it.
#define BH1750_LUX_FACTOR 1.2f

// ---------------------------------------------------------------------------
// Wi-Fi access point (there is no Wi-Fi network in the garage, so the logger
// creates its own). Browse to http://192.168.4.1 (any hostname also works).
// ---------------------------------------------------------------------------
#define AP_SSID_PREFIX "HeadlightLux"  // the chip id is appended: HeadlightLux-1A2B3C
#define AP_PASSWORD "headlight"        // >= 8 chars, or "" for an open network
#define AP_CHANNEL 6

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
// Samples kept in RAM for the live chart (4 bytes each). When a run outgrows
// it, the RAM copy is decimated 2:1; the CSV on flash always keeps every sample.
#define LIVE_BUFFER_SAMPLES 3000

// Allowed sample intervals (ms) and the default.
#define DEFAULT_INTERVAL_MS 1000
#define MIN_INTERVAL_MS 1000  // the fine range needs ~0.67 s per conversion
#define MAX_INTERVAL_MS 60000

// Samples are appended to the CSV on flash at least this often, so a dead
// power bank costs at most this much data.
#define FLUSH_EVERY_MS 10000

// Window used for the "plateau" value stored with each finished run.
// (The web UI lets you recompute plateau with any window.)
#define PLATEAU_WINDOW_S 120

// Hard limit on run length, whatever the UI asks for.
#define MAX_RUN_S (6UL * 3600UL)

// Stop recording when LittleFS has less free space than this.
#define MIN_FREE_FS_BYTES 16384

// Ambient (lights-off) baseline measurement length.
#define AMBIENT_MEASURE_MS 3000
