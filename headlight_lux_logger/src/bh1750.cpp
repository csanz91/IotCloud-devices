#include "bh1750.h"

#include <Wire.h>

#include "config.h"

namespace {

const uint8_t CMD_POWER_ON = 0x01;
const uint8_t CMD_RESET = 0x07;
const uint8_t CMD_CONT_H_RES = 0x10;   // 1 lx/count at MTreg 69
const uint8_t CMD_CONT_H_RES2 = 0x11;  // 0.5 lx/count at MTreg 69

struct RangeDef {
  uint8_t mode;
  uint8_t mtreg;
  const char* name;
};

const RangeDef RANGES[BH1750::kNumRanges] = {
    {CMD_CONT_H_RES2, 254, "fine (<7k lx)"},
    {CMD_CONT_H_RES2, 69, "normal (<27k lx)"},
    {CMD_CONT_H_RES, 31, "bright (<120k lx)"},
};

const uint16_t SATURATION_COUNTS = 60000;  // ~92% of full scale
const uint8_t FAILS_BEFORE_ERROR = 3;
const uint32_t RETRY_MS = 2000;

}  // namespace

bool BH1750::command(uint8_t cmd) {
  Wire.beginTransmission(addr_);
  Wire.write(cmd);
  return Wire.endTransmission() == 0;
}

uint16_t BH1750::convMaxMs(uint8_t range) const {
  // Datasheet: H-res max conversion time is 180 ms at MTreg 69, scaling
  // linearly with MTreg. Add a small margin so we never read a stale result.
  return (uint16_t)((180UL * RANGES[range].mtreg + 68) / 69) + 10;
}

float BH1750::countsToLux(uint16_t raw, uint8_t range) const {
  float lux = (float)raw / BH1750_LUX_FACTOR * (69.0f / RANGES[range].mtreg);
  if (RANGES[range].mode == CMD_CONT_H_RES2) lux *= 0.5f;
  return lux;
}

const char* BH1750::rangeName() const { return RANGES[range_].name; }

float BH1750::resolution() const { return countsToLux(1, range_); }

bool BH1750::configure(uint8_t range, uint32_t nowMs) {
  const uint8_t mt = RANGES[range].mtreg;
  bool good = command(0x40 | (mt >> 5)) &&    // MTreg high bits
              command(0x60 | (mt & 0x1F)) &&  // MTreg low bits
              command(RANGES[range].mode);    // (re)start continuous mode
  range_ = range;
  // The register may still hold a result from the previous setting: wait for
  // two full conversions before trusting it.
  nextReadMs_ = nowMs + 2UL * convMaxMs(range);
  return good;
}

bool BH1750::begin() {
  const uint32_t now = millis();
  ok_ = command(CMD_POWER_ON) && command(CMD_RESET) && configure(0, now);
  failStreak_ = 0;
  if (!ok_) nextRetryMs_ = now + RETRY_MS;
  return ok_;
}

bool BH1750::update(uint32_t nowMs) {
  if (!ok_) {
    if ((int32_t)(nowMs - nextRetryMs_) >= 0) {
      if (!begin()) errors_++;
    }
    return false;
  }
  if ((int32_t)(nowMs - nextReadMs_) < 0) return false;
  nextReadMs_ = nowMs + convMaxMs(range_);

  if (Wire.requestFrom(addr_, (uint8_t)2) != 2) {
    while (Wire.available()) Wire.read();
    errors_++;
    if (++failStreak_ >= FAILS_BEFORE_ERROR) {
      ok_ = false;
      nextRetryMs_ = nowMs + RETRY_MS;
    }
    return false;
  }
  failStreak_ = 0;
  uint16_t raw = ((uint16_t)Wire.read() << 8) | Wire.read();

  // Too bright for this range: switch down and drop the clipped reading.
  if (raw >= SATURATION_COUNTS && range_ + 1 < kNumRanges) {
    if (!configure(range_ + 1, nowMs)) errors_++;
    return false;
  }

  raw_ = raw;
  lux_ = countsToLux(raw, range_);
  saturated_ = raw >= SATURATION_COUNTS;  // only possible in the last range
  readings_++;

  // Dim enough for a more sensitive range: switch for the next readings, but
  // this reading is still valid.
  if (range_ > 0 && lux_ < 0.5f * maxLux(range_ - 1)) {
    if (!configure(range_ - 1, nowMs)) errors_++;
  }
  return true;
}
