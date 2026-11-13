#include "recorder.h"

#include <LittleFS.h>

namespace {

const char* RUNS_DIR = "/runs";
const char* CSV_HEADER = "elapsed_s,lux_net,lux_raw,range\n";

String fmtNum(float v, uint8_t decimals = 2) {
  if (isnan(v) || isinf(v)) return F("null");
  char buf[24];
  snprintf(buf, sizeof(buf), "%.*f", decimals, (double)v);
  return String(buf);
}

// Very small extractor for the flat JSON objects this file writes itself.
bool jsonField(const String& s, const char* key, String& out) {
  String pat = String('"') + key + "\":";
  int p = s.indexOf(pat);
  if (p < 0) return false;
  p += pat.length();
  if (p < (int)s.length() && s[p] == '"') {
    int e = s.indexOf('"', p + 1);
    if (e < 0) return false;
    out = s.substring(p + 1, e);
  } else {
    int e = p;
    while (e < (int)s.length() && s[e] != ',' && s[e] != '}') e++;
    out = s.substring(p, e);
  }
  return true;
}

float jsonFloat(const String& s, const char* key, float def) {
  String v;
  if (!jsonField(s, key, v) || v == "null") return def;
  return v.toFloat();
}

uint32_t jsonUint(const String& s, const char* key, uint32_t def) {
  String v;
  if (!jsonField(s, key, v)) return def;
  return strtoul(v.c_str(), nullptr, 10);
}

// Keep labels JSON- and CSV-safe: no quotes, backslashes or control chars;
// UTF-8 is kept but never cut in the middle of a character.
void sanitizeLabel(const String& in, char* out, size_t outSize) {
  size_t n = 0;
  for (size_t i = 0; i < in.length() && n + 1 < outSize; i++) {
    uint8_t c = (uint8_t)in[i];
    if (c < 0x20 || c == 0x7F || c == '"' || c == '\\') continue;
    out[n++] = (char)c;
  }
  // back off a truncated multi-byte sequence
  if (n + 1 >= outSize) {
    size_t k = n;
    while (k > 0 && ((uint8_t)out[k - 1] & 0xC0) == 0x80) k--;
    if (k > 0 && ((uint8_t)out[k - 1] & 0xC0) == 0xC0) n = k - 1;
  }
  out[n] = 0;
  // trim trailing spaces
  while (n > 0 && out[n - 1] == ' ') out[--n] = 0;
}

float meanOfRing(const float* ring, uint16_t cap, uint16_t head, uint16_t fill) {
  float sum = 0;
  uint16_t valid = 0;
  for (uint16_t i = 0; i < fill; i++) {
    float v = ring[(head + cap - 1 - i) % cap];
    if (!isnan(v)) {
      sum += v;
      valid++;
    }
  }
  return valid ? sum / valid : NAN;
}

}  // namespace

String Recorder::csvPath(uint32_t id) { return String(RUNS_DIR) + "/" + id + ".csv"; }
String Recorder::metaPath(uint32_t id) { return String(RUNS_DIR) + "/" + id + ".json"; }

String Recorder::metaJson(const RunMeta& m) {
  String s;
  s.reserve(220);
  s += F("{\"id\":");
  s += m.id;
  s += F(",\"label\":\"");
  s += m.label;
  s += F("\",\"start\":");
  s += m.startEpoch;
  s += F(",\"interval\":");
  s += m.intervalMs;
  s += F(",\"ambient\":");
  s += fmtNum(m.ambient);
  s += F(",\"trigger\":");
  s += fmtNum(m.trigger);
  s += F(",\"n\":");
  s += m.samples;
  s += F(",\"dur\":");
  s += fmtNum(m.durationS, 1);
  s += F(",\"plateau\":");
  s += fmtNum(m.plateau);
  s += F(",\"max\":");
  s += fmtNum(m.maxLux);
  s += F(",\"done\":");
  s += m.done ? 1 : 0;
  s += F(",\"recovered\":");
  s += m.recovered ? 1 : 0;
  s += '}';
  return s;
}

bool Recorder::deleteRun(uint32_t id) {
  bool a = LittleFS.remove(csvPath(id));
  bool b = LittleFS.remove(metaPath(id));
  return a || b;
}

void Recorder::begin() {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount failed, formatting"));
    LittleFS.format();
    LittleFS.begin();
  }
  if (!LittleFS.exists(RUNS_DIR)) LittleFS.mkdir(RUNS_DIR);

  uint32_t maxId = 0;
  Dir dir = LittleFS.openDir(RUNS_DIR);
  while (dir.next()) {
    uint32_t id = strtoul(dir.fileName().c_str(), nullptr, 10);
    if (id > maxId) maxId = id;
  }
  nextId_ = maxId + 1;
  recoverRuns();
}

// ---------------------------------------------------------------------------
// Power-loss recovery: a run whose meta still says done:0 was interrupted.
// Rebuild its summary from the CSV.
// ---------------------------------------------------------------------------
void Recorder::recoverRuns() {
  // collect ids first; don't modify the directory while iterating it
  uint32_t ids[32];
  uint8_t n = 0;
  Dir dir = LittleFS.openDir(RUNS_DIR);
  while (dir.next() && n < 32) {
    const String name = dir.fileName();
    if (!name.endsWith(".json")) continue;
    File f = dir.openFile("r");
    String s = f.readString();
    f.close();
    if (s.indexOf(F("\"done\":0")) >= 0) ids[n++] = strtoul(name.c_str(), nullptr, 10);
  }
  for (uint8_t i = 0; i < n; i++) {
    Serial.printf("Recovering interrupted run %u: %s\n", ids[i], recoverRun(ids[i]) ? "ok" : "failed");
  }
}

bool Recorder::recoverRun(uint32_t id) {
  File mf = LittleFS.open(metaPath(id), "r");
  if (!mf) return false;
  String js = mf.readString();
  mf.close();

  RunMeta m;
  m.id = id;
  String label;
  jsonField(js, "label", label);
  sanitizeLabel(label, m.label, sizeof(m.label));
  m.startEpoch = jsonUint(js, "start", 0);
  m.intervalMs = constrain(jsonUint(js, "interval", DEFAULT_INTERVAL_MS), (uint32_t)MIN_INTERVAL_MS,
                           (uint32_t)MAX_INTERVAL_MS);
  m.ambient = jsonFloat(js, "ambient", 0);
  m.trigger = jsonFloat(js, "trigger", 0);

  const uint16_t len = max<uint16_t>(1, min<uint32_t>(kPlateauCap, PLATEAU_WINDOW_S * 1000UL / m.intervalMs));
  uint16_t head = 0, fill = 0;
  float lastElapsed = -1;

  File cf = LittleFS.open(csvPath(id), "r");
  if (cf) {
    char line[64];
    uint8_t ll = 0;
    bool header = true;
    auto handleLine = [&](char* l) {
      if (header) {
        header = false;
        return;
      }
      char* comma = strchr(l, ',');
      if (!comma) return;
      lastElapsed = strtof(l, nullptr);
      char* end = nullptr;
      float net = strtof(comma + 1, &end);
      if (end == comma + 1) net = NAN;  // empty field = no reading
      m.samples++;
      if (!isnan(net) && (isnan(m.maxLux) || net > m.maxLux)) m.maxLux = net;
      plateauRing_[head] = net;
      head = (head + 1) % len;
      if (fill < len) fill++;
    };
    uint8_t buf[256];
    while (cf.available()) {
      int got = cf.read(buf, sizeof(buf));
      for (int i = 0; i < got; i++) {
        char c = (char)buf[i];
        if (c == '\n') {
          line[ll] = 0;
          handleLine(line);
          ll = 0;
        } else if (ll < sizeof(line) - 1) {
          line[ll++] = c;
        }
      }
      yield();
    }
    // a partial last line (power cut mid-write) is ignored on purpose
    cf.close();
  }
  m.plateau = meanOfRing(plateauRing_, len, head, fill);
  m.durationS = lastElapsed < 0 ? 0 : lastElapsed + m.intervalMs / 1000.0f;
  m.done = true;
  m.recovered = true;

  File out = LittleFS.open(metaPath(id), "w");
  if (!out) return false;
  out.print(metaJson(m));
  out.close();
  return true;
}

// ---------------------------------------------------------------------------
// Recording
// ---------------------------------------------------------------------------
bool Recorder::start(const String& label, uint32_t intervalMs, uint32_t autostopS, float trigger,
                     uint32_t epochNow, String& err) {
  if (state_ != IDLE) {
    err = F("already recording or armed");
    return false;
  }
  FSInfo info;
  LittleFS.info(info);
  if (info.totalBytes - info.usedBytes < MIN_FREE_FS_BYTES * 2) {
    err = F("flash is full, delete some runs");
    return false;
  }

  meta_ = RunMeta();
  meta_.id = nextId_;
  sanitizeLabel(label, meta_.label, sizeof(meta_.label));
  if (!meta_.label[0]) snprintf(meta_.label, sizeof(meta_.label), "Run %u", meta_.id);
  meta_.intervalMs = constrain(intervalMs, (uint32_t)MIN_INTERVAL_MS, (uint32_t)MAX_INTERVAL_MS);
  meta_.ambient = ambient_;
  meta_.trigger = trigger > 0 ? trigger : 0;
  meta_.startEpoch = epochNow;  // refined when a trigger fires
  autostopS_ = (autostopS == 0 || autostopS > MAX_RUN_S) ? MAX_RUN_S : autostopS;
  lastError_ = "";
  startMs_ = millis();  // arm time; startRecording() shifts startEpoch by the wait

  if (meta_.trigger > 0) {
    state_ = ARMED;
  } else {
    startRecording(millis());
  }
  if (state_ == IDLE) {  // startRecording failed
    err = lastError_;
    return false;
  }
  return true;
}

void Recorder::startRecording(uint32_t nowMs) {
  File f = LittleFS.open(csvPath(meta_.id), "w");
  if (!f) {
    lastError_ = F("cannot create run file");
    state_ = IDLE;
    return;
  }
  f.print(CSV_HEADER);
  f.close();

  if (meta_.startEpoch) meta_.startEpoch += (nowMs - startMs_) / 1000;  // time spent armed
  startMs_ = nowMs;
  nextSampleMs_ = nowMs + meta_.intervalMs;
  lastFlushMs_ = nowMs;
  pending_ = "";
  pending_.reserve(512);
  accSum_ = 0;
  accCount_ = 0;
  accRange_ = -1;
  plateauLen_ = max<uint16_t>(1, min<uint32_t>(kPlateauCap, PLATEAU_WINDOW_S * 1000UL / meta_.intervalMs));
  plateauHead_ = plateauFill_ = 0;
  liveCount_ = 0;
  liveStride_ = 1;
  groupSum_ = 0;
  groupValid_ = groupFill_ = 0;

  if (!writeMeta()) {
    LittleFS.remove(csvPath(meta_.id));
    lastError_ = F("cannot write run metadata");
    state_ = IDLE;
    return;
  }
  nextId_ = meta_.id + 1;
  state_ = RECORDING;
}

bool Recorder::writeMeta() {
  File f = LittleFS.open(metaPath(meta_.id), "w");
  if (!f) return false;
  f.print(metaJson(meta_));
  f.close();
  return true;
}

void Recorder::addReading(float lux, uint8_t range, uint32_t nowMs) {
  if (ambientUntilMs_) {
    ambSum_ += lux;
    ambCount_++;
  }
  if (state_ == ARMED) {
    // The triggering conversion was only partly lit (the lamp came on during
    // it), so it starts the run but is not averaged into sample 0.
    if (lux - ambient_ >= meta_.trigger) startRecording(nowMs);
    return;
  }
  if (state_ == RECORDING) {
    accSum_ += lux;
    accCount_++;
    accRange_ = range;
  }
}

void Recorder::closeSample(float raw, int8_t range) {
  const float net = isnan(raw) ? NAN : raw - meta_.ambient;
  const float elapsed = (float)meta_.samples * meta_.intervalMs / 1000.0f;

  char row[48];
  if (isnan(raw)) {
    snprintf(row, sizeof(row), "%.1f,,,\n", (double)elapsed);
  } else {
    snprintf(row, sizeof(row), "%.1f,%.2f,%.2f,%d\n", (double)elapsed, (double)net, (double)raw, range);
  }
  pending_ += row;

  meta_.samples++;
  meta_.durationS = (float)meta_.samples * meta_.intervalMs / 1000.0f;
  if (!isnan(net) && (isnan(meta_.maxLux) || net > meta_.maxLux)) meta_.maxLux = net;

  plateauRing_[plateauHead_] = net;
  plateauHead_ = (plateauHead_ + 1) % plateauLen_;
  if (plateauFill_ < plateauLen_) plateauFill_++;

  pushLive(net);
}

void Recorder::pushLive(float v) {
  if (!isnan(v)) {
    groupSum_ += v;
    groupValid_++;
  }
  if (++groupFill_ < liveStride_) return;

  if (liveCount_ >= LIVE_BUFFER_SAMPLES) {
    // decimate 2:1 and double the stride
    uint16_t half = liveCount_ / 2;
    for (uint16_t i = 0; i < half; i++) {
      float a = live_[2 * i], b = live_[2 * i + 1];
      live_[i] = isnan(a) ? b : (isnan(b) ? a : (a + b) / 2);
    }
    liveCount_ = half;
    liveStride_ *= 2;
    // the group just closed now only fills half a new-stride entry; keep
    // accumulating so live entries stay aligned to stride boundaries
    if (groupFill_ < liveStride_) return;
  }
  live_[liveCount_++] = groupValid_ ? groupSum_ / groupValid_ : NAN;
  groupSum_ = 0;
  groupValid_ = groupFill_ = 0;
}

void Recorder::flush() {
  if (!pending_.length()) return;
  File f = LittleFS.open(csvPath(meta_.id), "a");
  if (!f) {
    lastError_ = F("write failed, run stopped");
    state_ = IDLE;  // nothing else we can do
    return;
  }
  f.print(pending_);
  f.close();
  pending_ = "";
}

void Recorder::loop(uint32_t nowMs) {
  if (ambientUntilMs_ && (int32_t)(nowMs - ambientUntilMs_) >= 0) {
    if (ambCount_) ambient_ = ambSum_ / ambCount_;
    ambientUntilMs_ = 0;
  }

  if (state_ != RECORDING) return;

  while ((int32_t)(nowMs - nextSampleMs_) >= 0) {
    closeSample(accCount_ ? accSum_ / accCount_ : NAN, accRange_);
    accSum_ = 0;
    accCount_ = 0;
    accRange_ = -1;
    nextSampleMs_ += meta_.intervalMs;
    if (meta_.durationS >= autostopS_) {
      stop(nowMs);
      return;
    }
  }

  if ((int32_t)(nowMs - lastFlushMs_) >= FLUSH_EVERY_MS) {
    lastFlushMs_ = nowMs;
    flush();
    FSInfo info;
    LittleFS.info(info);
    if (info.totalBytes - info.usedBytes < MIN_FREE_FS_BYTES) {
      lastError_ = F("flash full, run stopped");
      stop(nowMs);
    }
  }
}

void Recorder::stop(uint32_t nowMs) {
  (void)nowMs;
  if (state_ == ARMED) {
    state_ = IDLE;  // never triggered: nothing was written
    return;
  }
  if (state_ != RECORDING) return;
  flush();
  meta_.plateau = meanOfRing(plateauRing_, plateauLen_, plateauHead_, plateauFill_);
  meta_.done = true;
  writeMeta();
  lastStoppedId_ = meta_.id;
  state_ = IDLE;
}

void Recorder::measureAmbient(uint32_t nowMs) {
  ambSum_ = 0;
  ambCount_ = 0;
  ambientUntilMs_ = nowMs + AMBIENT_MEASURE_MS;
  if (!ambientUntilMs_) ambientUntilMs_ = 1;
}

float Recorder::elapsedS(uint32_t nowMs) const {
  return state_ == RECORDING ? (nowMs - startMs_) / 1000.0f : 0;
}
