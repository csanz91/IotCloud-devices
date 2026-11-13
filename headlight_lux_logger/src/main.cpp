// Headlight lux logger
// ESP8266 (NodeMCU) + BH1750 (GY-302). Creates its own Wi-Fi access point and
// serves a self-contained web UI (no internet needed) to record, chart and
// compare headlight warm-up curves.

#include <Arduino.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <Wire.h>

#include "bh1750.h"
#include "config.h"
#include "recorder.h"
#include "web_ui.h"

namespace {

const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_MASK(255, 255, 255, 0);
const uint32_t STALE_READING_MS = 3000;

ESP8266WebServer server(80);
DNSServer dns;
BH1750 sensor(BH1750_ADDR);
Recorder rec;

String apSsid;
uint32_t epochBase = 0;  // unix seconds at millis() == 0, set by the browser
float lastLux = NAN;
uint32_t lastLuxMs = 0;

uint32_t epochNow() { return epochBase ? epochBase + millis() / 1000 : 0; }

String num(float v, uint8_t decimals = 2) {
  if (isnan(v) || isinf(v)) return F("null");
  char buf[24];
  snprintf(buf, sizeof(buf), "%.*f", decimals, (double)v);
  return String(buf);
}

void sendJson(int code, const String& body) {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  server.send(code, F("application/json"), body);
}

void sendError(int code, const String& msg) { sendJson(code, String(F("{\"error\":\"")) + msg + F("\"}")); }

void sendOk() { sendJson(200, F("{\"ok\":true}")); }

const char* stateName(Recorder::State s) {
  switch (s) {
    case Recorder::ARMED:
      return "armed";
    case Recorder::RECORDING:
      return "rec";
    default:
      return "idle";
  }
}

// ---------------------------------------------------------------------------
// HTTP handlers
// ---------------------------------------------------------------------------
void handleIndex() {
  server.sendHeader(F("Cache-Control"), F("no-cache"));
  server.send_P(200, PSTR("text/html; charset=utf-8"), INDEX_HTML, sizeof(INDEX_HTML) - 1);
}

void handleStatus() {
  const uint32_t now = millis();
  const bool fresh = sensor.ok() && !isnan(lastLux) && now - lastLuxMs < STALE_READING_MS;
  const RunMeta& r = rec.run();
  FSInfo fs;
  LittleFS.info(fs);

  String s;
  s.reserve(600);
  s += F("{\"ok\":");
  s += sensor.ok() ? F("true") : F("false");
  s += F(",\"fresh\":");
  s += fresh ? F("true") : F("false");
  s += F(",\"sat\":");
  s += sensor.saturated() ? F("true") : F("false");
  s += F(",\"lux\":");
  s += fresh ? num(lastLux) : F("null");
  s += F(",\"net\":");
  s += fresh ? num(lastLux - rec.ambient()) : F("null");
  s += F(",\"range\":\"");
  s += sensor.rangeName();
  s += F("\",\"res\":");
  s += num(sensor.resolution(), 3);
  s += F(",\"reads\":");
  s += sensor.readings();
  s += F(",\"i2cErr\":");
  s += sensor.errors();
  s += F(",\"amb\":");
  s += num(rec.ambient());
  s += F(",\"ambBusy\":");
  s += rec.ambientBusy() ? F("true") : F("false");
  s += F(",\"state\":\"");
  s += stateName(rec.state());
  s += F("\",\"elapsed\":");
  s += num(rec.elapsedS(now), 1);
  s += F(",\"autostop\":");
  s += rec.autostopS();
  s += F(",\"run\":");
  s += Recorder::metaJson(r);
  s += F(",\"lastId\":");
  s += rec.lastStoppedId();
  s += F(",\"err\":\"");
  s += rec.lastError();
  s += F("\",\"fsUsed\":");
  s += (uint32_t)fs.usedBytes;
  s += F(",\"fsTotal\":");
  s += (uint32_t)fs.totalBytes;
  s += F(",\"heap\":");
  s += ESP.getFreeHeap();
  s += F(",\"clock\":");
  s += epochBase ? F("true") : F("false");
  s += F(",\"up\":");
  s += now / 1000;
  s += '}';
  sendJson(200, s);
}

// Live samples of the active (or last) run, from index `from`. If the client's
// run id or stride is out of date, everything is sent again from 0.
void handleLive() {
  const RunMeta& r = rec.run();
  uint32_t from = server.arg(F("from")).toInt();
  if ((uint32_t)server.arg(F("id")).toInt() != r.id || (uint32_t)server.arg(F("stride")).toInt() != rec.liveStride() ||
      from > rec.liveCount()) {
    from = 0;
  }

  server.sendHeader(F("Cache-Control"), F("no-store"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("application/json"), "");

  String chunk;
  chunk.reserve(1100);
  chunk += F("{\"id\":");
  chunk += r.id;
  chunk += F(",\"stride\":");
  chunk += rec.liveStride();
  chunk += F(",\"interval\":");
  chunk += r.intervalMs;
  chunk += F(",\"from\":");
  chunk += from;
  chunk += F(",\"d\":[");
  const uint16_t count = rec.liveCount();
  for (uint16_t i = from; i < count; i++) {
    if (i != from) chunk += ',';
    chunk += num(rec.liveAt(i));
    if (chunk.length() > 1000) {
      server.sendContent(chunk);
      chunk = "";
    }
  }
  chunk += F("]}");
  server.sendContent(chunk);
  server.sendContent("");  // end of chunked response
}

void handleRuns() {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("application/json"), "");
  const bool recording = rec.state() == Recorder::RECORDING;
  const uint32_t activeId = rec.run().id;

  String chunk = "[";
  bool first = true;
  Dir dir = LittleFS.openDir(F("/runs"));
  while (dir.next()) {
    const String name = dir.fileName();
    if (!name.endsWith(F(".json"))) continue;
    String js;
    if (recording && strtoul(name.c_str(), nullptr, 10) == activeId) {
      js = Recorder::metaJson(rec.run());  // the file only has the start-time meta
    } else {
      File f = dir.openFile("r");
      js = f.readString();
      f.close();
    }
    if (!js.startsWith("{")) continue;
    if (!first) chunk += ',';
    first = false;
    chunk += js;
    if (chunk.length() > 1000) {
      server.sendContent(chunk);
      chunk = "";
    }
  }
  chunk += ']';
  server.sendContent(chunk);
  server.sendContent("");
}

void handleRunCsv() {
  const uint32_t id = server.arg(F("id")).toInt();
  if (rec.state() == Recorder::RECORDING && rec.run().id == id) rec.flush();
  File f = LittleFS.open(Recorder::csvPath(id), "r");
  if (!f) return sendError(404, F("no such run"));

  // filename: run_<id>_<label>.csv with only safe characters
  String fname = String(F("run_")) + id;
  File mf = LittleFS.open(Recorder::metaPath(id), "r");
  if (mf) {
    String js = mf.readString();
    mf.close();
    int p = js.indexOf(F("\"label\":\""));
    if (p >= 0) {
      p += 9;
      String safe;
      for (int i = p; i < (int)js.length() && js[i] != '"' && safe.length() < 40; i++) {
        char c = js[i];
        if (isalnum((unsigned char)c) || c == '-' || c == '_') safe += c;
        else if (c == ' ' && !safe.endsWith("_")) safe += '_';
      }
      if (safe.length()) fname += "_" + safe;
    }
  }
  server.sendHeader(F("Content-Disposition"), String(F("attachment; filename=\"")) + fname + F(".csv\""));
  server.sendHeader(F("Cache-Control"), F("no-store"));
  server.streamFile(f, F("text/csv"));
  f.close();
}

void handleStart() {
  String err;
  const uint32_t interval = server.hasArg(F("interval")) ? server.arg(F("interval")).toInt() : DEFAULT_INTERVAL_MS;
  const uint32_t autostop = server.arg(F("autostop")).toInt();
  const float trigger = server.arg(F("trigger")).toFloat();
  if (trigger > 0) {
    if (!sensor.ok() || isnan(lastLux)) return sendError(409, F("sensor not working"));
    // Otherwise the run would start at once with a wrong t=0.
    if (lastLux - rec.ambient() >= trigger)
      return sendError(409, F("already above the trigger level: switch the lights off or measure ambient"));
  }
  if (!rec.start(server.arg(F("label")), interval, autostop, trigger, epochNow(), err)) return sendError(409, err);
  sendOk();
}

void handleStop() {
  rec.stop(millis());
  sendOk();
}

void handleAmbient() {
  if (server.arg(F("action")) == F("clear")) {
    rec.clearAmbient();
  } else {
    if (!sensor.ok()) return sendError(409, F("sensor not working"));
    rec.measureAmbient(millis());
  }
  sendOk();
}

void handleTime() {
  // browser sends Date.now() in ms
  const uint64_t ms = strtoull(server.arg(F("ms")).c_str(), nullptr, 10);
  const uint32_t epoch = (uint32_t)(ms / 1000ULL);
  if (epoch < 1600000000UL) return sendError(400, F("bad time"));
  epochBase = epoch - millis() / 1000;
  sendOk();
}

void handleDelete() {
  const uint32_t id = server.arg(F("id")).toInt();
  if (rec.state() == Recorder::RECORDING && rec.run().id == id) return sendError(409, F("stop the run first"));
  if (!Recorder::deleteRun(id)) return sendError(404, F("no such run"));
  sendOk();
}

void handleNotFound() {
  // Captive-portal style: send any foreign host to the UI.
  if (server.hostHeader() != AP_IP.toString()) {
    server.sendHeader(F("Location"), String(F("http://")) + AP_IP.toString() + "/");
    server.send(302, F("text/plain"), "");
    return;
  }
  server.send(404, F("text/plain"), F("not found"));
}

// ---------------------------------------------------------------------------
// Status LED: off idle, short blink every second recording, fast blink armed,
// solid on sensor error.
// ---------------------------------------------------------------------------
void updateLed(uint32_t now) {
#if !STATUS_LED_ENABLED
  (void)now;
  return;
#endif
  bool on;
  if (!sensor.ok()) {
    on = true;
  } else if (rec.state() == Recorder::RECORDING) {
    on = (now % 1000) < 60;
  } else if (rec.state() == Recorder::ARMED) {
    on = (now % 250) < 60;
  } else {
    on = false;
  }
  digitalWrite(STATUS_LED_PIN, on ? STATUS_LED_ON : STATUS_LED_OFF);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("Headlight lux logger"));
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, STATUS_LED_OFF);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  Serial.printf("BH1750 at 0x%02X: %s\n", BH1750_ADDR, sensor.begin() ? "ok" : "NOT FOUND (will keep retrying)");

  rec.begin();

  char suffix[8];
  snprintf(suffix, sizeof(suffix), "%06X", ESP.getChipId());
  apSsid = String(AP_SSID_PREFIX) + "-" + suffix;
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.softAPConfig(AP_IP, AP_IP, AP_MASK);
  WiFi.softAP(apSsid.c_str(), AP_PASSWORD, AP_CHANNEL);
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", AP_IP);
  Serial.printf("AP \"%s\" up, open http://%s/\n", apSsid.c_str(), AP_IP.toString().c_str());

  server.on(F("/"), HTTP_GET, handleIndex);
  server.on(F("/api/status"), HTTP_GET, handleStatus);
  server.on(F("/api/live"), HTTP_GET, handleLive);
  server.on(F("/api/runs"), HTTP_GET, handleRuns);
  server.on(F("/api/run.csv"), HTTP_GET, handleRunCsv);
  server.on(F("/api/start"), HTTP_POST, handleStart);
  server.on(F("/api/stop"), HTTP_POST, handleStop);
  server.on(F("/api/ambient"), HTTP_POST, handleAmbient);
  server.on(F("/api/time"), HTTP_POST, handleTime);
  server.on(F("/api/delete"), HTTP_POST, handleDelete);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() {
  const uint32_t now = millis();
  dns.processNextRequest();
  server.handleClient();

  if (sensor.update(now)) {
    lastLux = sensor.lux();
    lastLuxMs = now;
    rec.addReading(lastLux, sensor.range(), now);
  }
  rec.loop(now);
  updateLed(now);

  static uint32_t lastPrint = 0;
  if (now - lastPrint >= 2000) {
    lastPrint = now;
    if (sensor.ok() && !isnan(lastLux)) {
      Serial.printf("lux %.2f net %.2f range %u state %s n %u\n", (double)lastLux, (double)(lastLux - rec.ambient()),
                    sensor.range(), stateName(rec.state()), rec.run().samples);
    } else {
      Serial.println(F("sensor not responding"));
    }
  }
}
