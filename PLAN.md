# Coriolis

Successor to Borealis (Aurora → Borealis → Coriolis). An all-new LED matrix
wall display: new hardware, new software, none of the 64KB-era constraints.
Borealis remains on the wall as a finished product.

Planning started July 2026.

## Decided

- **Separate project** — own hardware, own repo, no Borealis compatibility burden.
- **Display: 128x128 square, chunky pixels — DECIDED (July 2026).** Santtu is
  "totally sure" the final form is square: four 64x64 panels at P4 (~51x51 cm)
  or P5 (~64x64 cm). Square shows every content shape well (landscape,
  portrait, square) and makes rotation lossless. The 128x64 widescreen build
  remains as a sim experiment flag (`-DWIDE=ON`); code stays
  resolution-agnostic regardless.
- **Rotation is software-only.** No physical flip rack needed: on a square
  display a 90-degree software rotation (applied by the backend when copying
  the framebuffer out) is pixel-perfect identical to physically rotating the
  panel. Portrait-native content like Tetris just requests its orientation.
  Already prototyped in the sim (R key).
- **Features, in priority order:**
  1. Clock / weather / ambient info (weather implies a network data source —
     see platform notes)
  2. Audio-reactive patterns (I2S microphone)
  3. Games with real input (IR remote lag killed games on Borealis)
  4. **Yoga routine player** (see below — high enthusiasm, girlfriend-approved)

## Feature: yoga routine player

Animated yoga poses on the display, played as configurable routines.

- **Poses as skeletons, not sprites**: each pose is a set of joint
  coordinates for a stick figure. The renderer draws the figure with thick
  lines and a head circle — and because poses are keyframes, transitions
  tween smoothly from pose to pose. Scales to any display size and looks
  right in chunky pixels.
- **Routines**: named sequences of (pose, hold-duration). Configurable
  length, speed/pace, pose order (fixed or shuffled).
- **Sound**: a chime marks each pose change; a voice announces the pose
  name. Audio via a DFPlayer Mini (~4 EUR serial MP3 module + tiny speaker,
  own microSD with pre-recorded clips) — keeps the device offline, offloads
  all audio, one UART pin. Voice clips can be generated once with any TTS
  and copied to the card. In the simulator: raylib plays the same clips.
- Scene flow: select routine -> countdown -> poses with progress indicator
  (reuse the clock's seconds-bar idiom) -> done screen.

## Games (each is a Scene; input abstraction already supports them)

Confirmed wanted: **Pong** (vs. AI and the PongClock variant where the score
is the time — Borealis had this pattern), Snake, Tetris (portrait via
software rotation).

Good candidates, roughly easiest-first: Breakout, Game of Life (was a
Borealis pattern), falling-sand toy (Adafruit PixelDust-style, mesmerizing
at 128x128), Space Invaders (Borealis had the sprite art), Flappy-style
one-button game, Simon (color memory), 2048, Asteroids.

Ambient non-games that ride the same machinery: aquarium (fish tank),
fireplace mode (fire scene + crackle audio from the DFPlayer), Matrix rain,
starfield, bouncing-DVD-logo, pixel-art photo frames from SD.

**DOOM (the stretch trophy):** genuinely possible, not a joke — doomgeneric
ports need ~5 callbacks and people have run DOOM on a Teensy 4.1 with the
PSRAM upgrade (the WAD needs a few MB). At 128x128 it's a downscaled,
barely-playable glorious party trick — worth doing *because* it's absurd.
Requires: Teensy 4.1 + 8MB PSRAM soldered (+2 EUR) + gamepad. This is
another point for the Teensy platform choice. Parked until the core
features exist.
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
| 4x 64x64 P4 (or P5) HUB75 panels | ~€30-45 ea | Same scan type, one batch — final form is 128x128 |
| 5V PSU, 40A | ~€35-45 | Mean Well LRS-200-5 class; fuse + per-panel injection |
| DFPlayer Mini + small speaker | ~€6 | yoga chime/voice, fireplace crackle; own microSD |
| 8MB PSRAM chip for Teensy 4.1 | ~€3 | solder-on; big GIFs and the DOOM trophy |
| I2S MEMS mic (INMP441/SPH0645) | ~€5 | audio-reactive |
| ESP32 mini (UART weather/net co-processor) | ~€5 | or Ethernet magjack |
| USB wireless gamepad + dongle | ~€20 | games |
| Light sensor (photoresistor or TSL2591) | ~€2-8 | auto-brightness |
| Coin cell for 4.1 RTC, frame extrusion, wiring, connectors | ~€20-30 | |

Rough total: ~€280-330 for the full 128x128 build.

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
