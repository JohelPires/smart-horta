# Electronics notes (Phase 1)

Notes for a backend dev learning electronics, oriented at the Phase 1 wokwi.com
exercises (LED + resistor + button). Analogies to code where useful.

## 1. Voltage, current, resistance

- **Voltage (V)** = electrical potential difference, the "why electrons move" —
  think of it like a *pressure* or a log-level difference that drives flow.
  ESP32 output: **3.3 V** between a HIGH pin and GND.
- **Current (I or A)** = flow rate of electrons. Think *requests/second*.
  LED needs ~2–20 mA. GPIOs supply only ~12 mA safely (see §5).
- **Resistance (Ω)** = how much a component resists flow. Think *rate limiting*.
  A resistor converts surplus voltage into heat, and caps the current.

**Ohm's law** — the whole trade in one formula:

```
V = I · R        I = V / R        R = V / I
```

## 2. Power, and why the LED needs a resistor

Power dissipated (as heat): `P = V · I`.

An LED is a **diode**: it conducts one way (anode → cathode) and drops a nearly
fixed voltage when on — its **forward voltage** (`Vf`), typically ~2.0 V for red,
~3.2 V for blue/white. It is *not* a resistor: its own resistance collapses once
the voltage passes `Vf`, so nothing limits the current — up to or beyond levels
that destroy the LED (and maybe the GPIO).

The series resistor does the current limiting. Math for the Phase 1 circuit
(ESP32 GPIO @ 3.3 V, red LED `Vf ≈ 2.0 V`, resistor 330 Ω):

```
V_resistor = 3.3 − 2.0 = 1.3 V
I = V / R = 1.3 / 330 ≈ 3.9 mA
P_resistor = 1.3 · 0.0039 ≈ 5 mW   (heat, invisible — fine)
```

3.9 mA is dim-ish but completely safe for both LED and GPIO. Typical rule of
thumb: size the resistor so the LED gets 5–15 mA and the GPIO stays ≤ 12 mA
(§5). Common beginner mistake ⚠️: connecting an LED with *no* resistor — the
LED can be destroyed, and the GPIO may be damaged by the over-current too.

## 3. Digital logic: HIGH / LOW — and the 3.3 V rule

- **HIGH** = pin logic level 1 ≈ 3.3 V for a HIGH output / a valid HIGH input.
- **LOW** = 0 V.
- Inputs have a threshold (~1.8 V-ish region is undefined/noise; don't rely on it).

⚠️ **ESP32 GPIOs are NOT 5V-tolerant.** 5 V into a pin kills it (permanent
damage, no error, no fuse). Only feed ESP32 inputs 3.3 V signals. If you must
read a 5 V signal (rare here), use a **voltage divider** (§6) or a level shifter.

## 4. Ground: a common reference

Electricity is measured in *differences*. GND is the 0 V reference all
measurements are relative to. Every subsystem — ESP32 board, sensors, relay
supply, 12 V pump supply — must share a **common ground**, otherwise the
 Voltages between them are meaningless and signals won't register.

⚠️ Common beginner mistake: powering the sensor's 3.3 V rail from the ESP32 but
grounding it elsewhere (or forgetting to wire its GND back to the ESP32's GND).
Otherwise the measured "HIGH" may be nowhere near the ESP32's 3.3 V threshold.

## 5. GPIO current limits

- Per-pin: **~12 mA safe**, 40 mA absolute max (transient only).
- Total across pins also limited by the chip's package/power (stay well under).
- Never power motors, pumps, solenoids, or relay *coils* from a GPIO ⚠️ — use
  the GPIO as a *control signal* into a driver (transistor/relay board), with
  the load fed from a separate supply. GPIO = command path, not power path.

## 6. Voltage divider

Two resistors in series split the input voltage proportionally:

```
Vout = Vin · R2 / (R1 + R2)
```

where R1 is the top resistor (from Vin to the tap) and R2 goes from the tap to
GND. Uses in this project:

- **Reading analog sensors** that output resistance (NTC thermistor, resistive
  soil probe): pair the probe with a fixed resistor and read the tap voltage
  with the ESP32's ADC (Phase 4).
- **Level shifting** 5 V → 3.3 V: e.g. Vin = 5 V, R1 = 15 kΩ, R2 = 30 kΩ
  (5 · 30/45 = 3.3 V). Only for *slow/DC* signals.

## 7. The button circuit: pull-ups and floating inputs

A GPIO configured as `INPUT` with nothing connected is **floating**: it picks
up ambient/capacitive charge and reads random HIGH/LOW as you approach it
(the antenna effect). Never trust floating inputs ⚠️ — always define the
default state with a resistor.

**Pull-up** (most common for buttons): resistor from the pin to 3.3 V. Button
wired from the pin to GND. Reading:

- Released: pin = HIGH (pulled to 3.3 V)
- Pressed: pin = LOW (shorted to GND; current = 3.3 V / R — negligible)

The ESP32 has **configurable internal pull-ups/pull-downs** (`gpio_pullup_en`);
external 10 kΩ works too. Reverse everything for a pull-down + button-to-3.3 V
wiring (then released = LOW, pressed = HIGH).

Phase 1 recipe (wokwi.com): `wokwi-pushbutton` between GPIO 23 and GND,
`gpio_pullup_en` on the pin — released prints 1, pressed prints 0.

## 8. Flyback diode: inductive loads bite back

A pump, solenoid valve or relay coil is an **inductor**. While energized it
stores energy as a magnetic field; the instant you open the switch, the
collapsing field tries to keep current flowing and generates a *large reverse
voltage spike* (tens to hundreds of volts) — enough to kill a transistor or the
ESP32.

Fix: a **flyback (freewheeling) diode** connected reverse-parallel across the
coil — normal operation blocks it; turn-off gives the spike a safe local loop
for the current to decay. Relay *boards* usually include one (check the
schematic); bare solenoids/pumps in Phase 8 need one you add.

## 9. Isolation: relays and optocouplers

The garden valve runs at 12/24 V with its own power supply; the ESP32 runs at
3.3 V and must never touch that current path. A **relay module** (like
Wokwi's `wokwi-relay-module`) isolates the two sides:

- The GPIO drives the board's **input LED through an optocoupler** — photo
  coupling means no electrical connection, one-way, immune to ground issues.
- The mechanical relay's coil + contacts live on the other side, with their own
  flyback diode, and switch the valve circuit.

For faster/silent/longer-life switching, an **SSR** (solid-state relay) replaces
the mechanical coil with semiconductors. Phase 5 covers the real wiring.

## 10. Safety rules (memorize before Phase 8)

- ⚠️ **Never switch or route mains (110/220 V) on a breadboard. Period.** Bread-
  boards make poor contact, and mains is lethal at distances breadboards mock.
  All valve/pump work is done at 12/24 V DC, low-current, on breadboards or
  screw terminals.
- One wire change at a time; disconnect power before rewiring.
- Measure (resistance/voltage) *before* powering on a new connection.
- When in doubt on a real board: check polarity and **common ground** first.
