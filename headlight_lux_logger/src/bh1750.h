#pragma once

#include <Arduino.h>

// Minimal non-blocking BH1750 driver with auto-ranging.
//
// Three ranges, from most to least sensitive:
//   0  H-res mode2, MTreg 254   0.11 lx/count   up to ~7 400 lx    ~0.66 s/conv
//   1  H-res mode2, MTreg  69   0.42 lx/count   up to ~27 300 lx   ~0.18 s/conv
//   2  H-res mode,  MTreg  31   1.86 lx/count   up to ~121 000 lx  ~0.08 s/conv
// It steps to a less sensitive range when the raw count nears saturation and
// back when the light would fit in less than half of the more sensitive range.
// The first conversion after a range change is discarded.
class BH1750 {
 public:
  static const uint8_t kNumRanges = 3;

  explicit BH1750(uint8_t addr) : addr_(addr) {}

  // Wire.begin() must have been called. Returns true if the sensor answered.
  bool begin();

  // Call often. Returns true when a new valid reading is available via lux().
  bool update(uint32_t nowMs);

  float lux() const { return lux_; }
  uint16_t raw() const { return raw_; }
  bool ok() const { return ok_; }
  bool saturated() const { return saturated_; }
  uint8_t range() const { return range_; }
  const char* rangeName() const;
  float resolution() const;  // lux per count in the current range
  uint32_t readings() const { return readings_; }
  uint32_t errors() const { return errors_; }

 private:
  bool command(uint8_t cmd);
  bool configure(uint8_t range, uint32_t nowMs);
  float countsToLux(uint16_t raw, uint8_t range) const;
  float maxLux(uint8_t range) const { return countsToLux(65535, range); }
  uint16_t convMaxMs(uint8_t range) const;

  uint8_t addr_;
  uint8_t range_ = 0;
  bool ok_ = false;
  bool saturated_ = false;
  float lux_ = 0;
  uint16_t raw_ = 0;
  uint8_t failStreak_ = 0;
  uint32_t nextReadMs_ = 0;
  uint32_t nextRetryMs_ = 0;
  uint32_t readings_ = 0;
  uint32_t errors_ = 0;
};
