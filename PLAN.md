# Coriolis

Successor to Borealis (Aurora → Borealis → Coriolis). An all-new LED matrix
wall display: new hardware, new software, none of the 64KB-era constraints.
Borealis remains on the wall as a finished product.

Planning started July 2026.

## Decided

- **Separate project** — own hardware, own repo, no Borealis compatibility burden.
- **Display: staged, chunky pixels.** Santtu likes the pixelated look, so
  larger pitch (P4/P5), not fine pitch. Two target shapes, kept open by
  buying identical 64x64 panels:
  - Stage A: **128x64 widescreen** (two 64x64 panels side by side) — best look
    on a wall. At P4: ~51x26 cm. At P5: ~64x32 cm.
  - Stage B (optional later): add two more panels → **128x128 square**
    (P4: ~51x51 cm, P5: ~64x64 cm).
  - Software is resolution-agnostic from day one (MATRIX_WIDTH/HEIGHT
    everywhere, as Borealis already did).
- **Features, in priority order:**
  1. Clock / weather / ambient info (weather implies a network data source —
     see platform notes)
  2. Audio-reactive patterns (I2S microphone)
  3. Games with real input (IR remote lag killed games on Borealis)
  4. (undecided extras — parked)
- **Development model: desktop-first.** The Borealis `sim/` shim proved the
  pattern code can compile and render on a PC. Coriolis inverts the
  architecture: patterns are written against a thin display/input interface,
  with three backends — desktop simulator (SDL/raylib), and the real hardware
  backend for whichever platform wins below. Consequence: **software
  development can start now, before any hardware is ordered.**

## Open: the platform decision (Teensy 4.1 vs ESP32-S3)

Santtu is split between these two; mulling. What the research says, mapped to
*his* actual requirements:

### Requirement-by-requirement

| Requirement | Teensy 4.1 + SmartLED Shield V5 | ESP32-S3 + HUB75-DMA lib |
|---|---|---|
| 128x64 widescreen | Easy, 240 Hz, 36-48-bit color | Fine — this is the lib's sweet spot |
| **128x128 kept open** | Easy: ~120 Hz @ 48-bit / ~168 Hz @ 36-bit | **Marginal.** The library README itself warns resolutions beyond 128x64 risk crashes on classic ESP32 (SRAM limits); S3 can host the DMA buffer in octal PSRAM but output caps ~13 MHz → flicker limits |
| **Low-brightness quality** (Santtu runs minimum brightness) | SmartMatrix's high refresh depth = smooth gradients in dark rooms | BCM at reduced depth on large panels = visible banding exactly at low brightness |
| Easiest port of Aurora-style code | Small migration (SmartMatrix 3→4) | Full port to Adafruit_GFX-style API (sim shim is the bridge, so doable) |
| Games input | **USB Host port on the 4.1 — plug a wireless gamepad dongle straight in** | Bluetooth gamepad or phone/WiFi |
| Audio-reactive | **Teensy Audio Library is best-in-class** (I2S mic + FFT objects) | Works (I2S + arduinoFFT), less polished |
| Weather/network | No WiFi. Mitigations: (a) ESP32 as €5 UART co-processor feeding the existing serial-JSON protocol Borealis already had, (b) Teensy 4.1 native Ethernet (magjack ~€5) | Built-in WiFi — cleanest here |
| Ecosystem trajectory | Fading: SmartMatrix last release 2020, shield end-of-line | Thriving, actively maintained |
| Sourcing | Teensy 4.1: in stock (SparkFun ~$31). **Shield: end-of-line but obtainable now** — Jameco has stock, Crowd Supply limited, Smart-Prototyping lists it; retired at Adafruit/SparkFun/Pimoroni. **If we commit: buy two.** Fallback: OSHW gerbers in SmartMatrix repo `/extras/hardware/` | Everywhere, ~€15-25 (MatrixPortal S3, ESP32-Trinity) |

### Current recommendation

**Teensy 4.1 + SmartLED Shield V5** — flipped from the earlier ESP32 lean, because
Santtu's answers changed the weights: keeping 128x128 open is a hard
requirement (ESP32's weakest point), he runs at minimum brightness (where
SmartMatrix's color depth visibly wins), games want the 4.1's USB Host, audio
wants the Teensy Audio Library, and WiFi/web UI was *not* among his chosen
features — the one ESP32 trump card he didn't play. The weather feed is a €5
UART co-processor. The real costs: buy shields now while they exist (two, for
spares), and accept the quieter ecosystem.

Decision checkpoint: order nothing until Santtu has mulled. If ESP32 wins
instead, everything in this plan still holds except the display backend and
the BOM.

## Draft BOM (Teensy variant, Stage A)

| Item | Est. | Notes |
|---|---|---|
| Teensy 4.1 | ~$31 | SparkFun, in stock |
| SmartLED Shield for Teensy 4 (x2 — spare) | ~$24 ea | Jameco / Crowd Supply / Smart-Prototyping, end-of-line |
| 2x 64x64 P4 (or P5) HUB75 panels | ~€30-45 ea | Same scan type; buy from one batch; +2 later for 128x128 |
| 5V PSU, 20A (Stage A) / 40A (Stage B) | ~€25-45 | Mean Well LRS-100-5 / LRS-200-5 class; fuse + per-panel injection |
| I2S MEMS mic (INMP441/SPH0645) | ~€5 | audio-reactive |
| ESP32 mini (UART weather/net co-processor) | ~€5 | or Ethernet magjack |
| USB wireless gamepad + dongle | ~€20 | games |
| Light sensor (photoresistor or TSL2591) | ~€2-8 | auto-brightness |
| Coin cell for 4.1 RTC, frame extrusion, wiring, connectors | ~€20-30 | |

Rough total Stage A: ~€200.

## Phases

1. **Mull platform** (Santtu) → order hardware when decided.
2. **Software skeleton, desktop-first** (can start immediately): repo
   scaffolding, display-interface layer, port the best Borealis patterns onto
   it, run in the simulator. PlatformIO + modern toolchain; ArduinoJson;
   GifDecoder (not the old GifPlayer); watchdog from day one.
3. **Bench bring-up**: controller + 2 panels on the desk, hardware backend,
   settings, brightness, remote/input.
4. **Features** in priority order: clock/weather feed → audio-reactive →
   games + gamepad.
5. **Frame + mounting + wall install.** Borealis keeps its spot; Coriolis
   gets its own.

## Lessons carried over from Borealis (10 years of service)

- Watchdog from day one — a wall appliance must self-recover, not wait months
  for a power cycle.
- No abandoned dependencies (aJson taught this); prefer maintained libs, pin
  versions in PlatformIO.
- Resolution-agnostic pattern code paid off in 2016 (32→64) and will again.
- Keep the serial-JSON control protocol idea — it made the device scriptable.
- Free-RAM telemetry from day one, not ten years in.
- The pixelated look is the point. Chunky pixels, smooth gradients.
