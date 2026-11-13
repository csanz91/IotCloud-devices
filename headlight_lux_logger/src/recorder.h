#pragma once

#include <Arduino.h>

#include "config.h"

// Metadata stored next to each run's CSV as /runs/<id>.json.
struct RunMeta {
  uint32_t id = 0;
  char label[41] = "";
  uint32_t startEpoch = 0;  // 0 when the browser never sent the time
  uint32_t intervalMs = DEFAULT_INTERVAL_MS;
  float ambient = 0;        // lux subtracted from every sample
  float trigger = 0;        // net lux that started the run, 0 = manual start
  uint32_t samples = 0;
  float durationS = 0;
  float plateau = NAN;      // mean of the last PLATEAU_WINDOW_S
  float maxLux = NAN;
  bool done = false;
  bool recovered = false;   // closed at boot after a power loss
};

// Turns sensor readings into averaged samples, keeps a RAM copy of the active
// run for the live chart and appends every sample to /runs/<id>.csv.
class Recorder {
 public:
  enum State { IDLE, ARMED, RECORDING };

  // Mounts LittleFS, finds the next run id and closes runs a power loss left
  // open.
  void begin();

  // Feed every valid sensor reading here, with the sensor range it came from.
  void addReading(float lux, uint8_t range, uint32_t nowMs);
  // Call every loop pass.
  void loop(uint32_t nowMs);

  // trigger <= 0: start now; otherwise arm and start when net lux >= trigger.
  bool start(const String& label, uint32_t intervalMs, uint32_t autostopS, float trigger,
             uint32_t epochNow, String& err);
  void stop(uint32_t nowMs);

  void measureAmbient(uint32_t nowMs);
  void clearAmbient() { ambient_ = 0; }
  bool ambientBusy() const { return ambientUntilMs_ != 0; }
  float ambient() const { return ambient_; }

  State state() const { return state_; }
  const RunMeta& run() const { return meta_; }
  uint32_t autostopS() const { return autostopS_; }
  float elapsedS(uint32_t nowMs) const;
  const String& lastError() const { return lastError_; }
  uint32_t lastStoppedId() const { return lastStoppedId_; }

  // Live buffer: entry i averages samples [i*stride, (i+1)*stride).
  uint16_t liveCount() const { return liveCount_; }
  uint16_t liveStride() const { return liveStride_; }
  float liveAt(uint16_t i) const { return live_[i]; }

  // Writes pending CSV rows to flash (used before a download of the active run).
  void flush();

  static String csvPath(uint32_t id);
  static String metaPath(uint32_t id);
  static String metaJson(const RunMeta& m);
  static bool deleteRun(uint32_t id);

 private:
  void startRecording(uint32_t nowMs);
  void closeSample(float value, int8_t range);
  void pushLive(float v);
  bool writeMeta();
  void recoverRuns();
  bool recoverRun(uint32_t id);

  State state_ = IDLE;
  RunMeta meta_;
  uint32_t nextId_ = 1;
  uint32_t lastStoppedId_ = 0;
  uint32_t autostopS_ = 0;
  uint32_t startMs_ = 0;
  uint32_t nextSampleMs_ = 0;
  uint32_t lastFlushMs_ = 0;
  String pending_;
  String lastError_;

  // readings within the current sample
  float accSum_ = 0;
  uint16_t accCount_ = 0;
  int8_t accRange_ = -1;  // range of the last reading in the sample

  // ambient measurement
  float ambient_ = 0;
  uint32_t ambientUntilMs_ = 0;
  float ambSum_ = 0;
  uint16_t ambCount_ = 0;

  // last PLATEAU_WINDOW_S of samples, for the plateau summary
  static const uint16_t kPlateauCap = (PLATEAU_WINDOW_S * 1000UL) / MIN_INTERVAL_MS;
  float plateauRing_[kPlateauCap];
  uint16_t plateauHead_ = 0;
  uint16_t plateauFill_ = 0;
  uint16_t plateauLen_ = 1;

  // RAM copy for the live chart (decimated when full)
  float live_[LIVE_BUFFER_SAMPLES];
  uint16_t liveCount_ = 0;
  uint16_t liveStride_ = 1;
  float groupSum_ = 0;
  uint16_t groupValid_ = 0;
  uint16_t groupFill_ = 0;
};
