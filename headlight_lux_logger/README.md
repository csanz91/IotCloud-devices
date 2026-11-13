# Headlight lux logger

This is standalone ESP8266 (NodeMCU) firmware that logs illuminance from a BH1750 (GY-302). You use it to compare
headlamps (left vs right, old vs new bulbs) by their warm-up curves and plateau values. There's no Wi-Fi network in the
garage, so the logger creates its own access point and serves a self-contained web page. The page uses no CDN and needs
no internet.

## Hardware

| BH1750 (GY-302) | NodeMCU          |
|-----------------|------------------|
| VCC             | 3V3              |
| GND             | G                |
| SDA             | D2 (GPIO4)       |
| SCL             | D1 (GPIO5)       |
| ADDR            | G (address 0x23) |

You can change the address and pins in [src/config.h](src/config.h) (`BH1750_ADDR`, `I2C_SDA_PIN`, `I2C_SCL_PIN`).

Power the board from a USB power bank. Many power banks switch off when the current draw stays below about
50–100 mA, and the ESP in AP mode sits near that. If yours does, use one with a low-current or always-on mode. The
data survives a shutdown (see *Power loss* below), but the run ends.

The on-board LED shows the state:
- Off: idle.
- Short blink every second: recording.
- Fast blink: armed and waiting for light.
- Solid on: the sensor isn't responding.

Keep the board, its blue LED and its red power LED out of the sensor's view. At low-beam light levels even a small LED
adds a measurable offset, and the status LED is only lit during runs, not while ambient is measured. Set
`STATUS_LED_ENABLED 0` in `config.h` to turn the status LED off.

## Build and flash

```bash
cd headlight_lux_logger
pio run -t upload
pio device monitor
```

That's all you need. The web page is compiled into the firmware, so there's no `uploadfs` step. The 2 MB LittleFS
partition holds the saved runs and is formatted on first boot.

## Using it

1. Join the Wi-Fi network `HeadlightLux-XXXXXX`. The password is `headlight` (both are set in `config.h`).
2. Open `http://192.168.4.1`. Any http address also redirects there.
   - Phones complain the network has no internet. Choose **Stay connected**.
   - If the page doesn't load, turn off mobile data.
   - iPhones may open a captive-portal sheet on their own. Close it and open the address in Safari, because CSV
     downloads usually don't work inside that sheet.
3. Measure the **ambient baseline**:
   - Headlights off, with everything else as it will be during the run.
   - Press **Measure**. It averages 3 s. Every new run stores this value and subtracts it from its samples.
4. Set up the run:
   - Enter a **label**, e.g. "Left low beam, old bulb".
   - Pick the sample interval. 1 s is the default, and samples average all readings inside the interval.
   - Pick the auto-stop time.
5. Set **Start when net lx ≥**:
   - `0` starts immediately.
   - A small threshold, e.g. 5 lx, arms the logger: t = 0 is the moment the lamp switches on, so warm-up curves from
     different runs line up exactly. The reading that fires the trigger was only partly lit, so it isn't averaged in.
   - Arming is refused if the light is already above the threshold, for example with the lamp still on or ambient not
     measured, because t = 0 would be wrong.
   - Runs started by hand still compare fine. The page finds the lamp-on moment (the first sample above 15% of the
     plateau) and measures Initial and Settled from there. **Align at lamp-on** shifts the curves to match.
6. Watch the live readout and curve. **Trend, last 2 min** shows the slope in %/min and says *stable* once it drops
   below 0.3 %/min.
7. Stop, or let auto-stop end the run. The finished run is ticked automatically in **Saved runs**. Tick other runs to
   overlay them.

### Comparison table (for the ticked runs, plus the live run)
- **Plateau**: mean over the last N minutes of each run. N is selectable: 1, 2, 5 or 10 min.
- **Δ ref**: plateau difference against the run marked *Ref*.
- **Lamp on**: when the lamp came on, measured from the start of the run.
- **Initial** and **Init→plat**: mean of the first 5 s after lamp-on, and how much the output changed while warming
  up. This is typically negative for LEDs, which lose output as they heat up, and positive for HID lamps.
- **Settled ±5% / ±2%**: time from lamp-on until the curve stays inside that band around the plateau.
- **Trend** and **Noise**: slope and standard deviation inside the plateau window.
- **Range**: the sensor range or ranges used in the plateau window. It's highlighted when the window mixes ranges.
  Compare plateaus measured in the same range when you can (see *Auto-ranging* below).
- **% of plateau**: normalizes each curve to its own plateau so you can compare the curve shapes.

Each run is saved as `/runs/<id>.csv` with metadata in `/runs/<id>.json`. The CSV columns are
`elapsed_s,lux_net,lux_raw,range`, where range is 0 fine, 1 normal or 2 bright.
**CSV** downloads a run. An empty value means no reading in that interval, for example while the sensor is changing
range.

### Power loss
Samples reach flash at least every 10 s. If the power bank dies mid-run, the run's summary is rebuilt from the CSV on
the next boot, and the run shows as *(recovered)*.

### Time
There's no RTC and no NTP. The page sends the phone's clock when it loads, which gives runs a date. Elapsed time, the
thing that matters, comes from the ESP's own timer.

## Measurement notes

- **Auto-ranging.** The fine range has 0.11 lx/count and reads up to about 7 klx. The firmware switches to coarser
  ranges, up to about 120 klx, only near saturation, and back below about 3.7 klx. A range switch can leave one
  sample empty.
  - Because of that hysteresis, a plateau between about 3.7 and 6.8 klx can be read in either range, depending on
    whether the run went higher earlier. The sensor isn't perfectly linear across ranges, so check the **Range**
    column.
  - Headlights on a wall at 5–10 m are normally well below 3.7 klx, so this rarely matters.
- **Absolute accuracy.** The BH1750 is only about ±20% absolute (see `BH1750_LUX_FACTOR`). Comparisons made with the
  same sensor in the same spot are far better than that, and that's what this logger is for.
- **Spectral response.** The sensor is roughly eye-weighted (photopic). Halogen vs LED comparisons are meaningful but
  not lab-grade.
- **Keep the geometry fixed.** Mount the sensor rigidly: tape it to the wall, facing the car, at the same point for
  every run. Mark the car's position, for example by its tyres. At 5–10 m, a few cm of aim or distance change matters.
  Illuminance falls with 1/d², so a 10 cm distance error at 5 m is a 4% error.
- **Keep supply voltage constant.** Halogen output depends steeply on voltage, roughly V^3.4. Engine running at about
  14.2 V vs off at about 12.4 V is a 50%+ difference. Do every run the same way, ideally with the engine idling.
- **Keep stray light constant.** Don't stand between the lamp and the sensor, and don't let the phone screen light the
  sensor. Measure ambient again if the garage lighting changes.
- **Glow after switch-off.** Halogen filaments still glow briefly after switch-off. Start a run only once the lamp is
  fully off.

## Files

- `src/config.h`: pins, address, AP name and password, logging limits.
- `src/bh1750.*`: non-blocking BH1750 driver with auto-ranging. It has no library dependency.
- `src/recorder.*`: sampling, live buffer, LittleFS storage and power-loss recovery.
- `src/main.cpp`: Wi-Fi AP, DNS catch-all, HTTP API.
- `src/web_ui.h`: the web page (HTML, CSS, and a canvas chart in plain JS).

### HTTP API
- `GET /api/status`
- `GET /api/live?id=&from=&stride=`
- `GET /api/runs`
- `GET /api/run.csv?id=`
- `POST /api/start` with `label`, `interval` (ms), `autostop` (s) and `trigger` (lx).
- `POST /api/stop`
- `POST /api/ambient` with `action` set to `measure` or `clear`.
- `POST /api/time` with `ms`.
- `POST /api/delete` with `id`.
