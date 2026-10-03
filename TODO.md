# Smart Garden — Roadmap / TODO

ESP32 smart-gardening controller. Written for a backend dev ramping up on
electronics, C, and embedded. Work top to bottom; each phase has a **Deliverable**
and **Exit criteria** you can verify in Wokwi before touching real hardware.

Legend: `[ ]` todo · `[x]` done · ⚠️ = common beginner mistake · 🧠 = concept to learn

Milestones:
- [ ] M1 — Blink an LED in Wokwi (Phase 0–2)
- [ ] M2 — Read a real sensor value and log it (Phase 3–4)
- [ ] M3 — Drive a relay/valve safely from a sensor reading (Phase 4–5)
- [ ] M4 — Publish telemetry over MQTT from Wokwi (Phase 6)
- [ ] M5 — Host app stores + displays telemetry (Phase 7)
- [ ] M6 — Same firmware runs on real hardware (Phase 8)
- [ ] M7 — Fail-safe irrigation + CI green (Phase 9)

---

## Phase 0 — Environment & tooling
🧠 ESP-IDF is a toolchain + build system; `idf.py` wraps CMake/ninja/esptool.
**Deliverable:** `idf.py --version` works in a fresh shell.
- [x] Install ESP-IDF v5.5.5 (`~/esp-idf`), run `install.sh esp32`, auto-source `export.sh` from `~/.bashrc` 🧠
- [x] Install VS Code + extensions: Espressif IDF, Wokwi Simulator, Serial Monitor
- [x] Use standalone `wokwi-cli` (installed at `~/bin/wokwi-cli`) — `idf.py wokwi` needs IDF ≥ 6.0, skipped
- [ ] Create Wokwi account; VS Code extension license (F1 -> "Wokwi: Request a new License")
- [ ] `export WOKWI_CLI_TOKEN=...` (for CI later; add to `~/.bashrc`, never commit)
- [x] Install Python 3.11+, `git` (Mosquitto deferred to Phase 7)
- [x] Scaffold repo dirs: `firmware/`, `host/`, `docs/`, `emulation/`
- [x] `cd firmware && idf.py set-target esp32`

---

## Phase 1 — Electronics fundamentals (breadboard + theory)
🧠 Start in Wokwi; no risk of burning parts. Learn by measuring.
**Deliverable:** a breadboard circuit with an LED + resistor + button that you can explain.
- [x] 🧠 Voltage (V), current (A), resistance (Ω), Ohm's law `V = I·R` → `docs/electronics-notes.md` §1
- [x] 🧠 Power `P = V·I`; why a resistor limits current to protect an LED → §2
- [x] 🧠 Digital logic: HIGH/LOW. **ESP32 GPIO = 3.3V, NOT 5V-tolerant** ⚠️ → §3
- [x] 🧠 Ground: all parts must share a common ground (GND) ⚠️ → §4
- [ ] Build an LED circuit in wokwi.com: ESP32 DevKit GPIO 4 → 330Ω resistor → red LED → GND (parts: `wokwi-led`, `wokwi-resistor`)
- [ ] Build a button circuit in wokwi.com: pushbutton GPIO 23 → GND, enable internal pull-up (released=1, pressed=0) ⚠️ learn floating inputs
- [x] 🧠 Current limits: each GPIO ≈ 12mA safe / 40mA absolute; never power motors from a GPIO ⚠️ → §5
- [x] 🧠 Voltage divider (used later by resistive sensors and level shifting) → §6
- [x] 🧠 Why inductive loads (pumps, solenoids) need a **flyback diode** → §8
- [x] 🧠 Isolation: optocouplers/relays keep high voltage away from the ESP32 → §9
- [x] ⚠️ Safety rule: never switch mains (110/220V) on a breadboard → §10

**Exit:** build both circuits in wokwi.com, then re-read `docs/electronics-notes.md` §2 + §7 and confirm you could explain them to someone else.

---

## Phase 2 — C & ESP-IDF foundations (blink in Wokwi)
🧠 C for backend devs: no GC, manual memory, pointers, `struct`, fixed-size types.
**Deliverable:** ESP-IDF blink app running in Wokwi.
- [ ] 🧠 C crash course: types, arrays, pointers, `struct`, `enum`, headers, `static`
- [ ] 🧠 `esp_err_t` return codes + `ESP_ERROR_CHECK`; `ESP_LOGI/W/E` logging
- [ ] Read ESP-IDF project anatomy: `main/CMakeLists.txt`, root `CMakeLists.txt`, `Kconfig`, `sdkconfig.defaults`
- [ ] 🧠 FreeRTOS basics: tasks (`xTaskCreate`), `vTaskDelay`, queues, semaphores
- [ ] Create `firmware/wokwi.toml` pointing at `build/flasher_args.json` + `build/smart_horta.elf`
- [ ] Create `firmware/diagram.json` (ESP32 DevKit + LED + resistor)
- [ ] Write blink code with `gpio_set_level` / `gpio_set_direction`; `idf.py build`
- [ ] Run via VS Code "Wokwi: Start Simulator" and confirm the LED blinks
- [ ] Run `wokwi-cli . --timeout 10000 --expect-text "..."` and see it pass
**Exit:** M1 done; clean build + simulated blink.

---

## Phase 3 — Digital sensors & GPIO inputs
🧠 Sensor "protocols": one-wire (DHT22), I2C, SPI. Start with one-wire.
**Deliverable:** temperature + humidity logged to serial every 2s.
- [ ] Add `wokwi-dht22` to `diagram.json`; wire VCC→3.3V, GND→GND, SDA→GPIO, + 10kΩ pull-up ⚠️
- [ ] Add the DHT driver as an ESP-IDF component (use "DHT sensor library for ESPx" behavior) 🧠
- [ ] Read temperature/humidity; validate against Wokwi sliders
- [ ] 🧠 DHT22 min sampling interval ≈ 2s ⚠️
- [ ] (optional) Add a pushbutton as a "manual override" input with debounce
- [ ] Read a PIR/motion or float switch later if desired
**Exit:** stable readings; values change when you drag Wokwi sliders.

---

## Phase 4 — Analog inputs, ADC & calibration
🧠 Analog ≠ digital: the ADC converts 0–3.3V to a number (ESP32 12-bit: 0–4095).
**Deliverable:** calibrated soil-moisture percentage from an analog sensor.
- [ ] ⚠️ Key gotcha: **ADC2 is unavailable while WiFi is active** → use **ADC1 (GPIO32–GPIO39)**
- [ ] 🧠 Configure ADC oneshot with 11dB attenuation for the full 0–3.3V range
- [ ] 🧠 Only GPIO34–39 are input-only (no internal pull-ups); GPIO32/33 have pull-ups
- [ ] Add `wokwi-potentiometer` (or `wokwi-slide-potentiometer`) as a soil-moisture proxy on GPIO34
- [ ] Read raw value; map to 0–100% with `raw → map()` and clamp
- [ ] 🧠 Understand sensor direction: resistive soil sensors often read HIGH = dry
- [ ] Calibrate: record raw "air/dry" and "water/wet" values → store in `sdkconfig`/Kconfig
- [ ] Add `wokwi-ntc-temperature-sensor` on another ADC1 pin (voltage divider) — optional
- [ ] Note: real capacitive soil sensors are preferred over resistive (they corrode) 🧠
**Exit:** M2 done; moisture % tracks the slider and is calibrated.

---

## Phase 5 — Actuators, power & fail-safe irrigation
🧠 GPIOs control *signals*; external power drives *loads*. Keep them separate.
**Deliverable:** sensor threshold opens/closes a valve in simulation, fails safe.
- [ ] 🧠 Relay vs MOSFET vs SSR; why a relay/SSR isolates the load
- [ ] Add `wokwi-relay-module` (default `npn` = **active-high**); control `IN` from a GPIO
- [ ] ⚠️ Many real relay boards are active-LOW; confirm your board and invert logic if needed
- [ ] ⚠️ A 3.3V GPIO cannot drive a 5V relay coil directly → use the relay board's driver/transistor
- [ ] (alternative) Add `wokwi-servo` to model a ball valve; use LEDC PWM 🧠
- [ ] Build a small state machine: IDLE → SENSING → IRRIGATING → COOLDOWN
- [ ] 🧠 Implement a watchdog + timeout: close valve if a task hangs ⚠️
- [ ] 🧠 Fail-safe on init error: valve must default to CLOSED (relay de-energized)
- [ ] Add a manual override button that is ignored/queued safely
- [ ] Define thresholds in Kconfig: moisture_low, moisture_high, max_run_time
**Exit:** M3 done; simulated valve opens below threshold and always closes on fault.

---

## Phase 6 — Networking, WiFi & MQTT (the simulation payoff)
🧠 MQTT = publish/subscribe via a broker; QoS levels; retained messages.
**Deliverable:** telemetry published + commands received from Wokwi.
- [ ] 🧠 WiFi STA mode; connect `Wokwi-GUEST`, password `""`, channel 6
- [ ] Add `esp-mqtt` (in-tree in ESP-IDF); configure broker URI
- [ ] For Wokwi: use a public broker (`broker.hivemq.com:1883` or `test.mosquitto.org:1883`)
- [ ] Publish JSON telemetry to `horta/<device_id>/telemetry` (temp, humidity, moisture, valve state)
- [ ] Subscribe to `horta/<device_id>/cmd`; handle `{"valve":"on|off","duration_s":N}`
- [ ] 🧠 Auto-reconnect: handle WiFi drop + MQTT disconnect events
- [ ] 🧠 Keep payloads small; define a stable JSON schema in `docs/mqtt-topics.md`
- [ ] Test with Wireshark: download `wokwi.pcap` and inspect the MQTT packets
**Exit:** M4 done; broker shows telemetry, commands toggle the valve.

---

## Phase 7 — Host / Raspberry Pi (Python)
🧠 Decouple ingestion, storage, and presentation.
**Deliverable:** host stores telemetry and prints/serves latest state.
- [ ] `python -m venv .venv && pip install paho-mqtt` → freeze to `host/requirements.txt`
- [ ] Write `host/main.py`: MQTT subscriber that logs to SQLite
- [ ] Define DB schema: `readings(ts, device_id, metric, value)` and `events(ts, type, detail)`
- [ ] Implement command publisher + simple CLI (`valve on/off`, `status`)
- [ ] (optional) dashboard: FastAPI + tiny HTML, or a curses/rich TUI
- [ ] Point subscriber at Wokwi's public broker; run alongside the simulator
- [ ] For a fully local loop: run Mosquitto, enable the **Wokwi Private Gateway**, set firmware broker to `host.wokwi.internal:1883`
- [ ] Add `pytest host/` with a fake publisher/subscriber; assert parsing + DB writes
**Exit:** M5 done; `pytest host/` green and telemetry persisted.

---

## Phase 8 — Real hardware bring-up
🧠 Simulation hides electrical reality: noise, power, cold joints, heat.
**Deliverable:** the same firmware flashes and runs on a physical ESP32.
- [ ] BOM shopping list (put in `docs/bom.md`):
  - [ ] ESP32 DevKitC (or DevKit v1)
  - [ ] Capacitive soil-moisture sensor(s), DHT22
  - [ ] Relay module or SSR rated for your valve voltage/current
  - [ ] Solenoid valve (12V/24V DC) or pump + **flyback diode**
  - [ ] Separate 12V/24V supply + buck converter for the ESP32 (5V)
  - [ ] Resistors, jumpers, breadboard, multimeter
- [ ] 🧠 Power architecture: one supply for logic, one for the load; **common ground**
- [ ] ⚠️ Never drain the valve/pump through the ESP32 regulator; brownouts = random resets
- [ ] Wire one sensor at a time; verify with the multimeter before powering ⚠️
- [ ] `idf.py -p /dev/ttyUSB0 flash monitor`; confirm boot logs + WiFi connect
- [ ] Re-calibrate ADC thresholds on real hardware (soil + water) and commit to Kconfig
- [ ] Add `board.h` pin map; verify it matches `diagram.json`
- [ ] Run a 24h soak test logging moisture + valve events
**Exit:** M6 done; real watering cycle triggered by real soil moisture.

---

## Phase 9 — Reliability, CI & docs
**Deliverable:** automated sim test + documentation a stranger can follow.
- [ ] Add a Wokwi scenario (`.scenario.yaml`) that asserts boot + WiFi + one telemetry publish
- [ ] GitHub Actions: build firmware + `wokwi-cli --expect-text` (free tier = 50 sim-min/month; keep short)
- [ ] 🧠 Test fail-safe paths: force sensor failure, assert valve stays closed
- [ ] Document wiring, calibration procedure, and MQTT contract in `docs/`
- [ ] Add a "factory reset default" that closes valves on unexpected reboot
- [ ] Record `WOKWI_CLI_TOKEN` as a CI secret (never commit it) ⚠️
**Exit:** M7 done; CI green and docs complete.

---

## Appendix A — ESP32 pin cheat-sheet
- ⚠️ **Not 5V-tolerant** — logic is 3.3V.
- ⚠️ GPIO6–11 are wired to SPI flash — **do not use**.
- ⚠️ Input-only: GPIO34–39 (no pull-ups). ADC1 = GPIO32–39 (use for analog with WiFi on).
- ⚠️ ADC2 pins conflict with WiFi.
- ⚠️ Strapping pins (GPIO0, 2, 4, 5, 12, 15) affect boot; be careful what you attach.
- UART0: TX=GPIO1, RX=GPIO3 (used by the serial monitor/flash).
- Default I2C: SDA=GPIO21, SCL=GPIO22. Default DAC: GPIO25/26.

## Appendix B — Learning resources
- ESP-IDF Programming Guide + API Reference
- Wokwi docs (ESP32, WiFi, Parts, CLI)
- "DHT sensor library for ESPx"
- Beginner electronics: Ohm's law + voltage dividers + relay/transistor tutorials
