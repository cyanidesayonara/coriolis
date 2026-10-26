# Building Coriolis — parts, wiring, assembly, bring-up

This is the hardware build guide: what to buy, where, how it wires
together, how to flash it and how to check each stage before the next.
The firmware compiles today (Teensyduino 1.62 + SmartMatrix 4, patched) and
every scene runs in the simulator, so the build is purely mechanical and
electrical. A printable version is in
[`Coriolis_Build_Guide.pdf`](Coriolis_Build_Guide.pdf).

Prices were checked in late October 2026 and are indicative (EUR, incl.
VAT where shown). Items marked *est.* are estimates.

---

## 1. Decide the size first

The display is four 64×64 HUB75 panels in a 2×2 square (128×128 pixels).
The pixel pitch sets the size:

| Pitch | Panel | Display (2×2) | Availability (Oct 2026) |
|---|---|---|---|
| P2.5 | 160 mm | 320 mm | Waveshare, common. Small for a wall. |
| **P3** | 192 mm | **384 mm** | Waveshare 64×64 P3 (RobotShop EU ~€33), Adafruit #4732. Easiest to source. |
| **P4** | 256 mm | **512 mm** | HackSpark (FR) ~€35, AliExpress. The planned size — **verify the scan rate** (below). |
| P5 | 320 mm | 640 mm | 64×64 P5 panels are rare; most P5 panels are 64×32. Not recommended. |

**Recommendation:** P4 if you can get panels confirmed as *HUB75E, 1/32
scan, address lines A–E*; otherwise P3. The firmware is identical for
both.

### Panel compatibility checklist (ask the seller before paying)

- 64×64, **1/32 scan**, HUB75**E** (the fifth address line, E, on HUB75 pin 8).
  The SmartLED Shield V5 drives E natively — no jumper.
- Driver chip: ICN2028, FM6124, DP5220 and similar plain shift registers are fine.
  **FM6126A** works but needs `SM_HUB75_OPTIONS_FM6126A_RESET_AT_START`
  (see §6). **ICN2038S, ICN2053, GS6238S** panels (and "AB" panels with only
  A/B address lines) **do not work** with SmartMatrix.
- Ask for a photo of the back showing the chips. Order **all four in one batch**
  so colors and wiring match. (HackSpark's listing says "1:16 scan" — ask
  them; a true 1/16-scan 64×64 needs a different panel type and can't use
  C-shape stacking.)

---

## 2. Shopping list

| # | Item | Qty | Recommended part / where | ≈ EUR |
|---|---|---|---|---|
| 1 | **SmartLED Shield for Teensy 4 (V5)** | 1 | [Smart Prototyping](https://www.smart-prototyping.com/SmartLED-Shield-for-Teensy-4) (ships from China). **Buy this first** — it's discontinued nearly everywhere else (Crowd Supply, SparkFun, Adafruit). Fallback: the open-source Gerbers + BOM in the [SmartMatrix repo](https://github.com/pixelmatix/SmartMatrix/tree/master/extras/hardware/Teensy4) (JLCPCB assembly). | 20–25 + VAT/shipping |
| 2 | Teensy 4.1 (with headers) | 1 | SparkFun DEV-16771 via BerryBase (DE), The Pi Hut, Electrokit, Polish shops | 32–49 |
| 3 | 64×64 HUB75E panels, 1/32 scan | 4 | See §1 (HackSpark P4 €35 ea., Waveshare P3 ~€33 ea., AliExpress €18–30 *est.*) | 120–140 |
| 4 | 5 V power supply | 1 | **Mean Well LRS-200-5** (5 V 40 A, fanless). LRS-150F-5 (22 A) is enough with a brightness cap. | 28–40 *est.* |
| 5 | IEC C14 inlet with switch + fuse holder, **T3.15 A** slow-blow fuse, 3-core mains lead | 1 | any electronics shop | 8–12 *est.* |
| 6 | DC wiring: 1.5 mm² red/black (2.5 mm² for a single trunk), ferrules, Wago lever connectors, 2× 1000–2200 µF 10 V capacitors | 1 lot | Amazon.es / local | 15–25 *est.* |
| 7 | DFPlayer Mini (genuine DFRobot DFR0299) | 1 | RobotShop EU €6.49, Arduino Store €6.74 | 7 |
| 8 | microSD card 8–32 GB (FAT32) | 1 | any | 5 |
| 9 | Speaker, 40 mm, 3 W, 4 Ω | 1 | any | 3–5 |
| 10 | IR receiver **TSOP38438** (or TSOP38238), 100 Ω resistor, 10 µF capacitor | 1 | Mouser / DigiKey / local | 2 |
| 11 | Remote: **Apple Remote** (aluminium A1294 or white A1156, used/clone) *or* the 17-key NEC "Arduino kit" remote | 1 | eBay / AliExpress / Amazon | 2–12 |
| 12 | CR2032 holder + cell (keeps the clock through power cuts) | 1 | any | 3 |
| 13 | 1 kΩ resistor (DFPlayer RX), hook-up wire, Dupont/JST leads | 1 lot | — | 3 |
| 14 | Frame: 2020 aluminium extrusion + corner brackets + M5 T-nuts, *or* a plywood shadow box | 1 | Amazon.es / local | 30–45 |
| 15 | Magnetic M3 panel mounts (12 mm pot magnets) + galvanised steel strip | ~16 | Adafruit #4631-style / AliExpress | 12–20 |
| 16 | Diffuser: 3 mm **smoked / "black LED" cast acrylic**, cut to size | 1 | Spanish *metacrilato a medida* shop, Leroy Merlin | 20–45 *est.* |
| | **Optional** | | | |
| 17 | Light sensor TSL2591 (Adafruit #1980) — reserved for auto-brightness | 1 | Pimoroni / Pi Hut | 7–8 |
| 18 | ESP32-C3 board — reserved for the weather feed | 1 | AliExpress / DevKit | 3–12 |
| 19 | PSRAM APS6404L 8 MB (not needed at 128×128) | 1 | BerryBase | 2.50 |

**Total ≈ €330–450**, panels being the largest share. The optional items are
not used by the current firmware: weather and auto-brightness are prepared
in software (the weather scene waits for data; night dimming is
schedule-based) but their drivers are future work.

---

## 3. How it fits together

```
 230 V ──[IEC inlet: switch + T3.15A fuse]── Mean Well LRS-200-5 ── 5 V rail
                                                │ (V+ / V−, 1.5 mm² per panel pair)
          ┌───────────────┬──────────────┬──────┴────────┬───────────────┐
       panel 1         panel 2        panel 3         panel 4      shield JST (VEXT/GND)
                                                                        │
  HUB75 ribbon: shield ─► panel 1 IN → OUT ─► panel 2 IN → … → panel 4  │
                                                                   Teensy 4.1 VIN
  DFPlayer VCC/GND ◄── 5 V rail directly          (VUSB–VIN pads cut on the Teensy)
```

### Teensy 4.1 pin map

The SmartLED Shield V5 uses pins **2 (OE), 3 (LAT), 6, 7 (CLK), 9, 10, 11,
12, 13** (panel data) and reserves 4, 5, 22 (APA102 output). Pin 13 is panel
data, so the Teensy's orange LED flickers — that's normal. Coriolis uses:

| Function | Teensy pin | Notes |
|---|---|---|
| IR receiver OUT | **16** | TSOP powered from **3.3 V** (never 5 V — Teensy pins aren't 5 V tolerant) |
| DFPlayer TX → | **0** (RX1) | direct |
| → DFPlayer RX | **1** (TX1) | through a **1 kΩ** series resistor |
| RTC battery | VBAT + GND | CR2032 |
| *(future)* light sensor | 18 (SDA) / 19 (SCL) | 3.3 V |
| *(future)* ESP32 weather | 14 (TX3) / 15 (RX3) | 3.3 V UART |

> Older Coriolis docs put the IR receiver on pin 2. **Pin 2 is the panel
> OE line** — don't use it.

### Power notes

- Four 64×64 panels draw **4–5 A each at full white** (16–20 A total). The
  LRS-200-5 (40 A) has ample headroom; with the 22 A LRS-150F-5, cap
  brightness at compile time (`-DCORIOLIS_MAX_BRIGHTNESS=180`).
  Typical clock use is a few amps.
- Run each panel's power pigtail straight to the PSU terminals (or one
  1.5 mm² pair per two panels). Put a 1000–2200 µF capacitor across each
  pair at the panels.
- The shield feeds the Teensy from its JST VEXT input through a 0.5 A
  diode. **Cut the VUSB–VIN pad bridge** on the underside of the Teensy so
  USB and the 5 V rail never fight. Afterwards the Teensy only runs when
  the PSU is on — flash it with the PSU on.
- Power the DFPlayer (and later the ESP32) from the 5 V rail directly, not
  from the Teensy.
- **Mains:** earth the PSU's ⏚ terminal and any metal frame; keep the
  mains terminals covered; use the fused, switched IEC inlet. If you're not
  comfortable wiring mains, have an electrician do this part.

---

## 4. Build steps

1. **Prepare the Teensy.** Solder headers if needed. Cut the VUSB–VIN pads.
   Solder the CR2032 holder to VBAT/GND.
2. **Shield.** Plug the Teensy into the shield (the shield covers the 4.0
   footprint; snap off its perforated tab to reach pins 24–41 if ever needed).
   Fit the JST power pigtail.
3. **IR receiver.** TSOP pins (lens facing you) are OUT · GND · Vs. Wire
   Vs → 100 Ω → 3.3 V, a 10 µF capacitor from Vs to GND at the sensor, OUT →
   pin 16. Mount it behind a small hole in the frame or diffuser edge.
4. **DFPlayer.** VCC → 5 V rail, GND → common ground, TX → pin 0, RX ← 1 kΩ ←
   pin 1, SPK1/SPK2 → speaker (neither side to ground).
5. **Bench test with one panel** before building the frame (§6).
6. **Frame.** For P4: four 2020 rails sized so the 512 mm panel square sits
   inside with ~2 mm play (≈ 550 mm outside), plus 2–3 cross-members behind
   the panel seams carrying steel strip for the magnetic mounts. Panels must
   butt seam-to-seam; adjust with the magnets before tightening. For a
   wooden box: 70–90 mm deep sides, MDF/ACM back with steel strips, French
   cleat for the wall (total ≈ 5–6 kg).
7. **Chain the panels** (§5), mount the electronics plate on a cross-member,
   PSU low in the frame with clearance around it, ventilation slots top and
   bottom.
8. **Diffuser.** Smoked acrylic directly on the panel faces gives crisp,
   high-contrast pixels; a grid louver (≤ 1 mm walls at P4) plus a tinted
   sheet gives square "pixels" if you prefer that look.

---

## 5. Panel layout and chaining

The firmware drives one chain of four panels as 128×128. How they're
chained is set by `CORIOLIS_PANEL_OPTIONS` at the top of `coriolis.ino`:

| Option | Physical layout (viewed from the front) |
|---|---|
| `SM_HUB75_OPTIONS_NONE` (default) | All four panels upright. The chain fills the **top row** first, then continues on the bottom row (a longer cable back across). |
| `SM_HUB75_OPTIONS_BOTTOM_TO_TOP_STACKING` | All upright, **bottom row** first. |
| `SM_HUB75_OPTIONS_C_SHAPE_STACKING` | Top row upright, **bottom row mounted upside-down**, the chain snakes down the side — shortest cables. |
| `... C_SHAPE | BOTTOM_TO_TOP` | Bottom row upright and first, top row upside-down. |

Don't guess — use the **test pattern** (serial `t`, §6): four numbered,
colored quadrants with an arrow in the top-left corner. When 1-2 / 3-4 read
in order and the arrow points at the top-left corner, the layout is right.
If the whole image is merely rotated, use **Settings → Rotate** instead.

---

## 6. Firmware and bring-up

### Toolchain (Windows)

```powershell
winget install ArduinoSA.CLI
powershell -File firmware/setup.ps1       # Teensy core, SmartMatrix 4 + patch, shared code library
arduino-cli compile --fqbn teensy:avr:teensy41 firmware/coriolis
arduino-cli upload  --fqbn teensy:avr:teensy41 firmware/coriolis
```

If your Windows user name has non-ASCII characters (e.g. `Nykänen`), GCC
can't find its own headers. Map the folders to a drive letter first
(`subst X: C:\path\to\coriolis`) and run from `X:`.

Options are set with `--build-property "build.extra_flags=-DCORIOLIS_PANEL_OPTIONS=..."`
or by editing the defines at the top of `coriolis.ino`:
`CORIOLIS_PANEL_OPTIONS`, `CORIOLIS_MAX_BRIGHTNESS`.

### Bring-up order

1. **Teensy alone over USB** (before cutting VUSB–VIN, or with the PSU on):
   flash, open the serial monitor at 115200 baud, type `i` — it prints the
   time and the active scene.
2. **One panel**: connect it as panel 1, power it, type `t` for the test
   pattern. Quadrant 1 (red) should appear. Blank or noise → wrong panel
   type or an FM6126A panel (set `SM_HUB75_OPTIONS_FM6126A_RESET_AT_START`).
3. **All four panels**: test pattern again; fix the chain option (§5).
   Type `t` again to leave it.
4. **IR**: press remote buttons; each frame prints `[IR] 0x........ -> n`.
   `n` = 0 means the code isn't mapped — add it in `RemoteMap::decode()`.
5. **Audio**: generate the SD card (`python tools/generate_voice_pack.py`,
   see [AUDIO.md](AUDIO.md)), insert it into the DFPlayer, open Yoga and
   start a session — you should hear the countdown ticks and the pose name.
6. **Time**: the Teensy Loader sets the RTC to your PC's local time on
   upload; the first boot of a new build converts that to UTC
   automatically. Check the clock; if needed adjust it in **Settings →
   Clock** or send `T<unix seconds UTC>` over serial, e.g. in PowerShell:
   `"T$([DateTimeOffset]::UtcNow.ToUnixTimeSeconds())"`.

### Serial bench commands (115200 baud)

`u d l r` arrows · `k` OK · `b` back · `m` menu · `s` settings · `c` clock
overlay · `n` next scene · `h` home · `t` test pattern · `i` info ·
`T<unix>` set the clock (UTC).

---

## 7. Troubleshooting

| Symptom | Likely cause / fix |
|---|---|
| Panels dark, Teensy LED flickers | Panel power missing, ribbon on the panel's OUT side, or FM6126A panels (§6). |
| Random pixels / garbage rows | 1/16 vs 1/32 scan mismatch; wrong panel type; long or poor ribbon cables. |
| Quadrants in the wrong place | Chain option (§5). Whole picture rotated → Settings → Rotate. |
| Colors swapped (red ↔ blue) | Some panels use RBG/BGR order — swap the channels in `blit()`. |
| Clock 1–2 h off after flashing | Set Zone/Summer in Settings → Clock, or adjust the hour there. |
| Clock resets after a power cut | CR2032 missing/flat on VBAT. |
| Remote does nothing | Receiver on 3.3 V and pin 16? Check `[IR]` lines on serial; map unknown codes. |
| Audio silent | SD card FAT32 with `01/`, `02/`, `ADVERT/` at the root; volume in Settings; 1 kΩ on RX. |
| Flicker at night | Expected at very low brightness with long chains; raise Night brightness a step. |
