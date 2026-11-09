#include "waxer_web_portal.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>Hot Pot Chain Waxer</title>
<style>
:root {
  --bg: #0f1318;
  --card: #181e26;
  --card-border: #262f3d;
  --text: #e6edf3;
  --text-muted: #8b949e;
  --wax-red: #e53935;
  --wax-red-glow: rgba(229, 57, 53, 0.35);
  --accent-gold: #ffb300;
  --accent-green: #00e676;
  --accent-green-glow: rgba(0, 230, 118, 0.3);
  --accent-blue: #00b0ff;
  --accent-blue-glow: rgba(0, 176, 255, 0.25);
  --accent-orange: #ff9100;
}
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; }
body { background: var(--bg); color: var(--text); padding: 16px; display: flex; justify-content: center; min-height: 100vh; }
.container { width: 100%; max-width: 520px; display: flex; flex-direction: column; gap: 16px; }
header { display: flex; justify-content: space-between; align-items: center; padding-bottom: 8px; border-bottom: 1px solid var(--card-border); }
.brand { display: flex; align-items: center; gap: 10px; }
.brand-badge { background: var(--wax-red); color: #fff; font-weight: 900; font-size: 11px; letter-spacing: 1.5px; padding: 4px 8px; border-radius: 4px; text-transform: uppercase; }
.brand-title { font-size: 17px; font-weight: 700; letter-spacing: 0.5px; }
.header-actions { display: flex; align-items: center; gap: 10px; }
.icon-btn { background: var(--card); border: 1px solid var(--card-border); color: var(--text); border-radius: 8px; padding: 6px 10px; font-size: 13px; cursor: pointer; display: flex; align-items: center; gap: 6px; }
.icon-btn.active { border-color: var(--wax-red); color: var(--wax-red); }

.card { background: var(--card); border: 1px solid var(--card-border); border-radius: 16px; padding: 20px; position: relative; box-shadow: 0 4px 20px rgba(0,0,0,0.35); }

/* Main Temp Card */
.temp-display { display: flex; flex-direction: column; align-items: center; text-align: center; gap: 6px; padding: 10px 0; }
.temp-val-wrap { display: flex; align-items: baseline; justify-content: center; position: relative; }
.temp-value { font-size: 80px; font-weight: 800; letter-spacing: -2px; line-height: 1; transition: color 0.3s; }
.temp-unit { font-size: 32px; font-weight: 600; color: var(--text-muted); margin-left: 4px; }
.target-pill { font-size: 14px; color: var(--text-muted); display: flex; align-items: center; gap: 8px; background: rgba(255,255,255,0.05); padding: 4px 12px; border-radius: 20px; margin-top: 4px; }
.target-diff { font-weight: 600; }
.target-diff.hot { color: var(--accent-orange); }
.target-diff.cold { color: var(--accent-blue); }
.target-diff.match { color: var(--accent-green); }

/* Status Banner */
.status-banner { margin-top: 12px; padding: 14px; border-radius: 12px; font-size: 14px; line-height: 1.4; font-weight: 500; display: flex; align-items: center; gap: 12px; transition: all 0.3s ease; }
.status-banner.heating_up { background: rgba(0, 176, 255, 0.12); border: 1px solid var(--accent-blue); color: var(--accent-blue); }
.status-banner.in_zone, .status-banner.ready_to_remove { background: rgba(0, 230, 118, 0.15); border: 1px solid var(--accent-green); color: var(--accent-green); box-shadow: 0 0 15px var(--accent-green-glow); }
.status-banner.cooling_down { background: rgba(255, 179, 0, 0.12); border: 1px solid var(--accent-gold); color: var(--accent-gold); }
.status-banner.too_hot { background: rgba(229, 57, 53, 0.15); border: 1px solid var(--wax-red); color: var(--wax-red); box-shadow: 0 0 15px var(--wax-red-glow); }
.status-banner.standby { background: rgba(255,255,255,0.05); border: 1px solid var(--card-border); color: var(--text-muted); }
.status-banner.sensor_error { background: rgba(229, 57, 53, 0.2); border: 1px solid var(--wax-red); color: #ff8a80; }
.status-icon { font-size: 22px; flex-shrink: 0; }

/* Stage Presets */
.section-title { font-size: 13px; font-weight: 700; text-transform: uppercase; letter-spacing: 1px; color: var(--text-muted); margin-bottom: 12px; }
.preset-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
.preset-btn { background: #1f2733; border: 1px solid var(--card-border); border-radius: 12px; padding: 14px; text-align: left; color: var(--text); cursor: pointer; transition: all 0.2s; position: relative; }
.preset-btn:hover { border-color: #3b485d; }
.preset-btn.active { border-color: var(--wax-red); background: rgba(229, 57, 53, 0.12); box-shadow: 0 0 12px var(--wax-red-glow); }
.preset-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 4px; }
.preset-name { font-size: 14px; font-weight: 700; }
.preset-temp { font-size: 18px; font-weight: 800; color: var(--wax-red); }
.preset-btn.active .preset-temp { color: #ff5252; }
.preset-desc { font-size: 11px; color: var(--text-muted); line-height: 1.3; }

/* Soak Timer Card */
.timer-card { display: flex; flex-direction: column; gap: 12px; }
.timer-row { display: flex; justify-content: space-between; align-items: center; }
.timer-display { font-size: 38px; font-weight: 800; font-family: monospace; letter-spacing: 1px; }
.timer-actions { display: flex; gap: 8px; }
.btn-sm { background: #263140; border: 1px solid var(--card-border); color: var(--text); padding: 8px 14px; border-radius: 8px; font-size: 13px; font-weight: 600; cursor: pointer; }
.btn-sm:hover { background: #324053; }
.progress-track { width: 100%; height: 8px; background: #222b37; border-radius: 4px; overflow: hidden; }
.progress-fill { height: 100%; background: linear-gradient(90deg, var(--wax-red), var(--accent-gold)); width: 0%; transition: width 0.5s linear; }
.timer-hint { font-size: 12px; color: var(--text-muted); display: flex; justify-content: space-between; }

/* Chart */
.chart-card { display: flex; flex-direction: column; gap: 10px; }
canvas#tempChart { width: 100%; height: 160px; background: #13171d; border-radius: 8px; }

/* Diagnostics & Custom Target */
.custom-input-row { display: flex; gap: 10px; margin-top: 10px; }
.custom-input-row input { flex: 1; background: #13171d; border: 1px solid var(--card-border); border-radius: 8px; color: #fff; padding: 10px; font-size: 14px; }
.custom-input-row button { background: var(--wax-red); border: none; color: #fff; padding: 10px 16px; border-radius: 8px; font-weight: 700; cursor: pointer; }
.diag-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; font-size: 12px; color: var(--text-muted); margin-top: 10px; padding-top: 10px; border-top: 1px solid var(--card-border); }
.diag-val { color: var(--text); font-weight: 600; }
</style>
</head>
<body>
<div class="container">
  <header>
    <div class="brand">
      <div class="brand-badge">HOT POT</div>
      <div class="brand-title">Chain Waxer</div>
    </div>
    <div class="header-actions">
      <button class="icon-btn active" id="audioBtn" onclick="toggleAudio()" title="Sound Alert">
        <span id="audioIcon">🔔</span>
      </button>
      <div id="wifiBadge" class="icon-btn" style="cursor:default">
        📶 <span id="wifiRssi">--</span>
      </div>
    </div>
  </header>

  <!-- Live Temperature Readout -->
  <div class="card">
    <div class="temp-display">
      <div class="temp-val-wrap">
        <span class="temp-value" id="currentTemp">--.-</span>
        <span class="temp-unit">&deg;C</span>
      </div>
      <div class="target-pill">
        <span>Target: <strong id="targetTemp">85.0&deg;C</strong></span>
        <span class="target-diff" id="targetDiff">--</span>
      </div>
    </div>
    <div class="status-banner heating_up" id="statusBanner">
      <span class="status-icon" id="statusIcon">🔥</span>
      <span id="guidanceText">Connecting to sensor...</span>
    </div>
  </div>

  <!-- Waxing Stages -->
  <div class="card">
    <div class="section-title">Waxing Stage Presets</div>
    <div class="preset-grid">
      <div class="preset-btn" id="preset-strip" onclick="selectStage('strip')">
        <div class="preset-header">
          <span class="preset-name">Strip Grease</span>
          <span class="preset-temp">125&deg;</span>
        </div>
        <div class="preset-desc">High-temp cycle to strip factory grease for new chains</div>
      </div>
      <div class="preset-btn active" id="preset-wax" onclick="selectStage('wax')">
        <div class="preset-header">
          <span class="preset-name">Regular Wax</span>
          <span class="preset-temp">85&deg;</span>
        </div>
        <div class="preset-desc">Hot melt wax immersion soak (penetration range 75-88&deg;C)</div>
      </div>
      <div class="preset-btn" id="preset-remove" onclick="selectStage('remove')">
        <div class="preset-header">
          <span class="preset-name">Pull Chain</span>
          <span class="preset-temp">72&deg;</span>
        </div>
        <div class="preset-desc">Optimal removal temp (70-75&deg;C) for max wax coat thickness</div>
      </div>
      <div class="preset-btn" id="preset-standby" onclick="selectStage('standby')">
        <div class="preset-header">
          <span class="preset-name">Standby</span>
          <span class="preset-temp" style="color:var(--text-muted)">OFF</span>
        </div>
        <div class="preset-desc">Monitor pot temperature without active setpoint alert</div>
      </div>
    </div>
    <div class="custom-input-row">
      <input type="number" id="customTempInput" placeholder="Custom Target &deg;C (40 - 130)" min="40" max="130" step="1">
      <button onclick="applyCustomTarget()">Set</button>
    </div>
  </div>

  <!-- Soak Timer -->
  <div class="card timer-card">
    <div class="section-title">Immersion / Soak Timer</div>
    <div class="timer-row">
      <div class="timer-display" id="timerDisplay">00:00</div>
      <div class="timer-actions">
        <button class="btn-sm" id="timerToggleBtn" onclick="toggleTimer()">Pause</button>
        <button class="btn-sm" onclick="resetTimer()">Reset</button>
      </div>
    </div>
    <div class="progress-track">
      <div class="progress-fill" id="timerProgress"></div>
    </div>
    <div class="timer-hint">
      <span>Target Soak: 10 mins with occasional agitation</span>
      <span id="soakPercent">0%</span>
    </div>
  </div>

  <!-- Realtime Temperature Chart -->
  <div class="card chart-card">
    <div class="section-title">Temperature Trend</div>
    <canvas id="tempChart" width="480" height="160"></canvas>
  </div>

  <!-- Diagnostics & Sensor Info -->
  <div class="card">
    <div class="section-title">Diagnostics & Calibration</div>
    <div class="diag-grid">
      <div>Sensor Address:</div>
      <div class="diag-val" id="sensorAddr">--</div>
      <div>Session Min / Max:</div>
      <div class="diag-val"><span id="minTemp">--</span> / <span id="maxTemp">--</span> &deg;C</div>
      <div>Uptime:</div>
      <div class="diag-val" id="uptime">--</div>
      <div>1-Wire Pin:</div>
      <div class="diag-val" id="wirePin">--</div>
    </div>
  </div>
</div>

<script>
let soundEnabled = true;
let audioCtx = null;
let lastStatus = null;
let history = [];
const MAX_HISTORY = 90; // ~90 seconds of points
let targetReachedPlayed = false;

function initAudio() {
  if (!audioCtx) {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  }
}

function playChime() {
  if (!soundEnabled) return;
  try {
    initAudio();
    const now = audioCtx.currentTime;
    const osc = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.connect(gain);
    gain.connect(audioCtx.destination);
    osc.type = 'sine';
    osc.frequency.setValueAtTime(587.33, now); // D5
    osc.frequency.exponentialRampToValueAtTime(880.0, now + 0.15); // A5
    gain.gain.setValueAtTime(0.2, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.5);
    osc.start(now);
    osc.stop(now + 0.5);
  } catch (e) {
    console.warn("Audio error", e);
  }
}

function toggleAudio() {
  soundEnabled = !soundEnabled;
  const btn = document.getElementById('audioBtn');
  const icon = document.getElementById('audioIcon');
  if (soundEnabled) {
    btn.classList.add('active');
    icon.innerText = '🔔';
    initAudio();
  } else {
    btn.classList.remove('active');
    icon.innerText = '🔕';
  }
}

async function fetchStatus() {
  try {
    const res = await fetch('/api/status');
    if (!res.ok) throw new Error("HTTP " + res.status);
    const data = await res.json();
    updateUI(data);
  } catch (err) {
    document.getElementById('guidanceText').innerText = "Reconnecting to Chain Waxer...";
    document.getElementById('statusBanner').className = "status-banner sensor_error";
    document.getElementById('statusIcon').innerText = "⚠️";
  }
}

function updateUI(data) {
  // Temp display
  const tempEl = document.getElementById('currentTemp');
  if (data.sensor_connected && !isNaN(data.temperature)) {
    tempEl.innerText = data.temperature.toFixed(1);
    // Add point to chart
    history.push({ time: Date.now(), temp: data.temperature, target: data.target });
    if (history.length > MAX_HISTORY) history.shift();
  } else {
    tempEl.innerText = "--.-";
  }

  // Target and diff
  const targetEl = document.getElementById('targetTemp');
  const diffEl = document.getElementById('targetDiff');
  if (data.stage === 'standby' || data.target <= 0) {
    targetEl.innerText = "Standby";
    diffEl.innerText = "";
  } else {
    targetEl.innerText = data.target.toFixed(1) + "°C";
    const diff = data.diff;
    if (Math.abs(diff) <= 2.5) {
      diffEl.innerText = "IN ZONE (±2°C)";
      diffEl.className = "target-diff match";
      if (!targetReachedPlayed) {
        playChime();
        targetReachedPlayed = true;
      }
    } else {
      targetReachedPlayed = false;
      if (diff > 0) {
        diffEl.innerText = "+" + diff.toFixed(1) + "°C Above Target";
        diffEl.className = "target-diff hot";
      } else {
        diffEl.innerText = diff.toFixed(1) + "°C Below Target";
        diffEl.className = "target-diff cold";
      }
    }
  }

  // Banner & Guidance
  const banner = document.getElementById('statusBanner');
  banner.className = "status-banner " + data.status;
  document.getElementById('guidanceText').innerText = data.guidance;
  const icons = {
    heating_up: "🔥",
    in_zone: "✅",
    ready_to_remove: "⛓️",
    cooling_down: "❄️",
    too_hot: "⚠️",
    too_cold: "❄️",
    standby: "⏸️",
    sensor_error: "❌"
  };
  document.getElementById('statusIcon').innerText = icons[data.status] || "ℹ️";

  // Stage buttons
  ['strip', 'wax', 'remove', 'standby'].forEach(st => {
    const btn = document.getElementById('preset-' + st);
    if (btn) {
      if (data.stage === st) btn.classList.add('active');
      else btn.classList.remove('active');
    }
  });

  // Timer
  const m = Math.floor(data.soak_seconds / 60);
  const s = data.soak_seconds % 60;
  document.getElementById('timerDisplay').innerText = 
    (m < 10 ? '0' : '') + m + ':' + (s < 10 ? '0' : '') + s;
  const pct = Math.min(100, Math.round((data.soak_seconds / 600) * 100));
  document.getElementById('timerProgress').style.width = pct + '%';
  document.getElementById('soakPercent').innerText = pct + '%';
  document.getElementById('timerToggleBtn').innerText = data.timer_running ? 'Pause' : 'Resume';

  // Diagnostics
  document.getElementById('sensorAddr').innerText = data.address || '--';
  document.getElementById('minTemp').innerText = isNaN(data.min_temp) ? '--' : data.min_temp.toFixed(1);
  document.getElementById('maxTemp').innerText = isNaN(data.max_temp) ? '--' : data.max_temp.toFixed(1);
  document.getElementById('wifiRssi').innerText = data.wifi_rssi + " dBm";
  document.getElementById('wirePin').innerText = data.pin_name || '--';

  const upM = Math.floor(data.uptime_s / 60);
  document.getElementById('uptime').innerText = upM + " min";

  renderChart(data.target);
}

function renderChart(currentTarget) {
  const canvas = document.getElementById('tempChart');
  const ctx = canvas.getContext('2d');
  const w = canvas.width;
  const h = canvas.height;
  ctx.clearRect(0, 0, w, h);

  if (history.length < 2) return;

  // Find min and max for chart scaling
  let minT = 150, maxT = 20;
  history.forEach(p => {
    if (p.temp < minT) minT = p.temp;
    if (p.temp > maxT) maxT = p.temp;
  });
  if (currentTarget > 0) {
    if (currentTarget < minT) minT = currentTarget;
    if (currentTarget > maxT) maxT = currentTarget;
  }
  minT = Math.floor(minT - 5);
  maxT = Math.ceil(maxT + 5);
  const range = (maxT - minT) || 1;

  // Draw grid lines
  ctx.strokeStyle = '#222b37';
  ctx.lineWidth = 1;
  ctx.beginPath();
  for (let i = 0; i <= 4; i++) {
    const y = h - (i / 4) * (h - 20) - 10;
    ctx.moveTo(35, y);
    ctx.lineTo(w - 10, y);
    ctx.fillStyle = '#6b7684';
    ctx.font = '10px monospace';
    const val = (minT + (i / 4) * range).toFixed(0);
    ctx.fillText(val + '°', 5, y + 3);
  }
  ctx.stroke();

  // Draw target dashed line if active
  if (currentTarget > 0) {
    const targetY = h - ((currentTarget - minT) / range) * (h - 20) - 10;
    ctx.strokeStyle = '#ffb300';
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(35, targetY);
    ctx.lineTo(w - 10, targetY);
    ctx.stroke();
    ctx.setLineDash([]);
  }

  // Draw temp curve
  ctx.strokeStyle = '#e53935';
  ctx.lineWidth = 2.5;
  ctx.beginPath();
  const stepX = (w - 45) / (MAX_HISTORY - 1);
  const startOffset = MAX_HISTORY - history.length;

  history.forEach((p, idx) => {
    const x = 35 + (startOffset + idx) * stepX;
    const y = h - ((p.temp - minT) / range) * (h - 20) - 10;
    if (idx === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();
}

async function selectStage(st) {
  try {
    await fetch('/api/stage?stage=' + encodeURIComponent(st), { method: 'POST' });
    fetchStatus();
  } catch(e) { console.error(e); }
}

async function applyCustomTarget() {
  const input = document.getElementById('customTempInput');
  const val = parseFloat(input.value);
  if (!isNaN(val) && val >= 40 && val <= 130) {
    await fetch('/api/target?temp=' + encodeURIComponent(val), { method: 'POST' });
    input.value = '';
    fetchStatus();
  }
}

async function toggleTimer() {
  await fetch('/api/timer/toggle', { method: 'POST' });
  fetchStatus();
}

async function resetTimer() {
  await fetch('/api/timer/reset', { method: 'POST' });
  fetchStatus();
}

// Polling interval 1000ms
setInterval(fetchStatus, 1000);
window.addEventListener('load', () => {
  fetchStatus();
  document.body.addEventListener('click', initAudio, { once: true });
});
</script>
</body>
</html>
)rawliteral";

WaxerWebPortal::WaxerWebPortal(ChainWaxerSensor &sensor)
    : _sensor(sensor)
{
    _server.on("/", HTTP_GET, [this]() { handle_root(); });
    _server.on("/api/status", HTTP_GET, [this]() { handle_api_status(); });
    _server.on("/api/stage", HTTP_POST, [this]() { handle_api_stage(); });
    _server.on("/api/target", HTTP_POST, [this]() { handle_api_target(); });
    _server.on("/api/timer/reset", HTTP_POST, [this]() { handle_api_timer_reset(); });
    _server.on("/api/timer/toggle", HTTP_POST, [this]() { handle_api_timer_toggle(); });
    _server.on("/api/offset", HTTP_POST, [this]() { handle_api_offset(); });
    _server.onNotFound([this]() { handle_not_found(); });
}

void WaxerWebPortal::begin()
{
    // Start mDNS responder
    if (MDNS.begin("chainwaxer"))
    {
        MDNS.addService("http", "tcp", HTTP_PORT);
        Serial.println(F("mDNS responder started: http://chainwaxer.local"));
    }
}

void WaxerWebPortal::loop()
{
    if (is_wifi_ready())
    {
        if (!_started)
        {
            start();
        }
        _server.handleClient();
        MDNS.update();
    }
    else if (_started)
    {
        stop();
    }
}

void WaxerWebPortal::start()
{
    _server.begin();
    _started = true;
    Serial.print(F("Chain Waxer web portal listening at http://"));
    Serial.println(WiFi.localIP());
}

void WaxerWebPortal::stop()
{
    _server.stop();
    _started = false;
}

bool WaxerWebPortal::is_wifi_ready() const
{
    // Only serve while connected to Wi-Fi. The access point exists only in
    // IotCloud setup mode, whose own setup page already uses port 80.
    const WiFiMode_t mode = WiFi.getMode();
    return (mode == WIFI_STA || mode == WIFI_AP_STA) && (WiFi.status() == WL_CONNECTED);
}

void WaxerWebPortal::handle_root()
{
    _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void WaxerWebPortal::handle_api_status()
{
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");

    const float temp = _sensor.get_current_temperature();
    const float target = _sensor.get_target_temperature();
    const float diff = _sensor.get_temp_difference();
    const char *stage_str = "standby";
    switch (_sensor.get_stage())
    {
    case WaxStage::STRIP_GREASE:
        stage_str = "strip";
        break;
    case WaxStage::REGULAR_WAX:
        stage_str = "wax";
        break;
    case WaxStage::CHAIN_REMOVAL:
        stage_str = "remove";
        break;
    case WaxStage::CUSTOM:
        stage_str = "custom";
        break;
    case WaxStage::STANDBY:
    default:
        stage_str = "standby";
        break;
    }

    char buffer[768];
    snprintf(
        buffer,
        sizeof(buffer),
        "{"
        "\"temperature\":%.2f,"
        "\"target\":%.2f,"
        "\"diff\":%.2f,"
        "\"stage\":\"%s\","
        "\"stage_name\":\"%s\","
        "\"status\":\"%s\","
        "\"guidance\":\"%s\","
        "\"soak_seconds\":%lu,"
        "\"timer_running\":%s,"
        "\"sensor_connected\":%s,"
        "\"address\":\"%s\","
        "\"pin_name\":\"%s\","
        "\"min_temp\":%.2f,"
        "\"max_temp\":%.2f,"
        "\"offset\":%.2f,"
        "\"wifi_rssi\":%ld,"
        "\"uptime_s\":%lu"
        "}",
        std::isnan(temp) ? -999.0f : temp,
        target,
        diff,
        stage_str,
        _sensor.get_stage_name(),
        _sensor.get_status_code(),
        _sensor.get_guidance_text(),
        static_cast<unsigned long>(_sensor.get_soak_seconds()),
        _sensor.is_timer_running() ? "true" : "false",
        _sensor.is_sensor_connected() ? "true" : "false",
        _sensor.get_sensor_address_hex().c_str(),
        _sensor.get_pin_name(),
        _sensor.get_min_temperature() > 900.0f ? 0.0f : _sensor.get_min_temperature(),
        _sensor.get_max_temperature() < -900.0f ? 0.0f : _sensor.get_max_temperature(),
        _sensor.get_calibration_offset(),
        static_cast<long>(WiFi.RSSI()),
        static_cast<unsigned long>(millis() / 1000UL));

    _server.send(200, "application/json", buffer);
}

void WaxerWebPortal::handle_api_stage()
{
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    if (_server.hasArg("stage"))
    {
        const String st = _server.arg("stage");
        if (st == "strip")
        {
            _sensor.set_stage(WaxStage::STRIP_GREASE);
        }
        else if (st == "wax")
        {
            _sensor.set_stage(WaxStage::REGULAR_WAX);
        }
        else if (st == "remove")
        {
            _sensor.set_stage(WaxStage::CHAIN_REMOVAL);
        }
        else if (st == "standby")
        {
            _sensor.set_stage(WaxStage::STANDBY);
        }
        _server.send(200, "text/plain", "OK");
        return;
    }
    _server.send(400, "text/plain", "Missing stage parameter");
}

void WaxerWebPortal::handle_api_target()
{
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    if (_server.hasArg("temp"))
    {
        const float val = _server.arg("temp").toFloat();
        if (val >= 40.0f && val <= 130.0f)
        {
            _sensor.set_custom_target(val);
            _server.send(200, "text/plain", "OK");
            return;
        }
    }
    _server.send(400, "text/plain", "Invalid or missing temp parameter");
}

void WaxerWebPortal::handle_api_timer_reset()
{
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _sensor.reset_soak_timer();
    _server.send(200, "text/plain", "OK");
}

void WaxerWebPortal::handle_api_timer_toggle()
{
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _sensor.set_timer_running(!_sensor.is_timer_running());
    _server.send(200, "text/plain", "OK");
}

void WaxerWebPortal::handle_api_offset()
{
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    if (_server.hasArg("val"))
    {
        const float val = _server.arg("val").toFloat();
        if (std::isfinite(val) && fabsf(val) <= MAX_OFFSET_C)
        {
            _sensor.set_calibration_offset(val);
            _server.send(200, "text/plain", "OK");
            return;
        }
    }
    _server.send(400, "text/plain", "Invalid or missing val parameter");
}

void WaxerWebPortal::handle_not_found()
{
    _server.send(404, "text/plain", "Not Found");
}
