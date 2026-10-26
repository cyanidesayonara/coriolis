# PROJECT.md — Coriolis

Successor to Borealis (Aurora → Borealis → Coriolis). An all-new LED matrix
wall display: new hardware, new software, none of the 64KB-era constraints.
Borealis remains on the wall as a finished product.

Planning started July 2026. Hardware build planned for late 2026 after house move.

## Architecture & Design Pillars

- **Display: 128×128 square, chunky pixels (Decided July 2026)**.
  Four 64×64 HUB75E (1/32 scan) panels at P4 (~51×51 cm) or P3 (~38×38 cm);
  64×64 P5 panels turned out to be rare, see `docs/BUILD.md`. Square shows every
  content shape well (landscape, portrait, square) and makes software rotation lossless.
  The 128×64 widescreen build remains as a sim experiment flag (`-DWIDE=ON`);
  code stays resolution-agnostic regardless.
- **Hardware Platform: Teensy 4.1 + SmartLED Shield V5**.
  - Refresh depth: 36/48-bit color, ~120 Hz @ 128×128. Smooth gradients at minimum brightness.
  - Built-in hardware RTC (persists through power cycles with a coin cell).
  - USB Host port on Teensy 4.1 for direct wireless gamepad dongles.
  - DFPlayer Mini MP3 serial module on Serial1 (pins 0/1) with its own microSD for offline audio.
  - Pins taken by the SmartLED Shield V5: 2 (OE), 3 (LAT), 6, 7 (CLK), 9–13; 4/5/22 reserved.
    The IR receiver goes on pin 16.
  - ESP32 mini UART co-processor for weather feeds (keeps display firmware completely offline).
- **Physical IR Remote Control**:
  - Apple Remote (aluminium A1294 / white A1156) or the 17-key NEC kit remote (`docs/REMOTE.md`).
  - Tap Menu = launcher, hold Menu = settings, hold Play/Pause = home; arrows + OK everywhere.
  - Mat-friendly for fitness: pause, skip/redo a step, adjust holds/reps on the fly.
- **Desktop-First Development**:
  - The desktop simulator (`sim/main.cpp` using raylib) is the primary development environment.
  - `scenes/shell.h` (splash, scene switching, launcher, settings, transitions, overlay, toasts,
    autoplay, night dimming) is shared verbatim by the simulator and the firmware, so the sim
    exercises the device's navigation code. Backends only map inputs and copy `shell.frame()`.
  - Scenes draw into an abstract `FrameBuffer` via `Context& ctx` and never touch hardware directly.

## Guided Fitness & Yoga Suite

Designed specifically to convincingly replace YouTube workout/yoga videos on a daily basis:
- **Glanceability from the mat**: High-contrast posture skeletons, massive timers, and color cues readable from any angle on the floor without craning your neck toward a phone or TV.
- **Bilateral Flow Engine**: Real yoga and kettlebell training require symmetrical bilateral execution. Asymmetrical movements (`Warrior 1 & 2`, `Tree`, `Triangle`, `Low Lunge`, `Pigeon`, `Lunges`, `KB Rows`, `KB Press`) automatically mirror across the vertical axis with anatomical joint swapping, accompanied by `(R)` and `(L)` side banners and audio side cues.
- **Next-Up HUD Previews**: 4 seconds before any transition, a high-visibility badge previews the next movement (`NEXT: DOWN DOG`) along with a soft countdown tick, so transitions are smooth and anticipated.
- **Audio Coaching**:
  - Start/finish bells for routine lifecycle.
  - Halfway audio cue (`Cue::Halfway`) when 50% of a pose hold is elapsed.
  - 3-2-1 countdown tick-down audio cues (`Cue::Tick`).
  - Spoken pose and rep count cues.
- **Expanded Routine Library**:
  - **Yoga**:
    1. *Morning Wake-Up* (Child's Pose → Cow → Cat → Down Dog → Low Lunge R/L → Fold → Mountain)
    2. *Sun Salutation* (Mountain → Fold → Plank → Cobra → Down Dog → Warrior 1 R/L → Fold → Mountain)
    3. *Standing Strength & Balance* (Chair → Warrior 1 & 2 R/L → Triangle R/L → Star → Tree R/L)
    4. *Hips & Back* (Child's Pose → Cat/Cow → Down Dog → Low Lunge R/L → Pigeon R/L → Bridge → Savasana)
    5. *Bedtime Restorative* (Child's Pose → Cat/Cow → Pigeon R/L → Bridge → Boat → Savasana)
  - **Fitness & Kettlebell**:
    1. *Bodyweight Circuit* (Squats, Pushups, Lunges R/L, Situps, Jacks, Glute Bridges)
    2. *Kettlebell Full-Body* (Swings, Goblet Squats, Rows R/L, Overhead Press R/L)
    3. *HIIT Tabata* (4 rounds of 20s work / 10s rest with full-screen emerald green / amber color cues)
    4. *Core & Pilates* (Situps, Bridges, Pushups, Plank hold)
- **Workout Lifecycle**:
  - Setup Card → 3-2-1 countdown → Active Routine with breathing figure → "Namaste" / "Great Job!" completion screen with total duration and celebratory chime.

## Scene Launcher & Navigation UX

- **Categorized Scene Launcher (`M` key / Remote Menu)**:
  - Jump directly across all 25 scenes without slow linear scrolling:
    - `CLOCKS & INFO`: Digital, Analog, Word, Weather, Calendar, Bounce
    - `GUIDED FITNESS`: Yoga, Exercise, Breathe, Focus (Pomodoro)
    - `GAMES`: Pong, Snake, Tetris, Labyrinth (Raycaster)
    - `AMBIENT ART`: Coriolis, Aurora, Lava, Spiro, Mandala, Fireplace, Aquarium, Starfield, Life, Rain, GIFs
- **Settings Menu**:
  - Jumps directly to the active scene's section when opened (`S` key).
  - Configures brightness, palettes, rotation, autoplay, workout programs, rep counts, and overlay styles.

## BOM (Teensy 4.1 Variant, 128×128) — checked October 2026

Full sourcing, wiring and bring-up: [`docs/BUILD.md`](docs/BUILD.md) /
[`docs/Coriolis_Build_Guide.pdf`](docs/Coriolis_Build_Guide.pdf).

| Item | Est. | Notes |
|---|---|---|
| SmartLED Shield for Teensy 4 (V5) | ~€20–25 + VAT | **Buy first** — discontinued at most stores; Smart Prototyping still lists it; open-source Gerbers as fallback |
| Teensy 4.1 | ~€32–49 | SparkFun DEV-16771 via EU resellers |
| 4× 64×64 HUB75E panels, 1/32 scan | ~€30–35 ea | P4 (HackSpark, verify scan) or P3 (Waveshare); avoid ICN2038S/ICN2053 drivers |
| 5V PSU | ~€28–40 | Mean Well LRS-200-5 (40 A) |
| IEC inlet + fuse, DC wiring, caps | ~€25–35 | |
| DFPlayer Mini + microSD + 3W speaker | ~€15 | |
| TSOP38438 IR receiver + Apple/NEC remote | ~€5–15 | receiver on pin 16 |
| CR2032 + holder | ~€3 | RTC backup |
| Frame (2020 extrusion or plywood), magnetic mounts, smoked acrylic | ~€60–110 | |
| *Optional*: TSL2591, ESP32-C3, PSRAM | ~€15 | not used by the current firmware |

Rough total: ~€330–450.

## Current Software Status (October 26, 2026)

- **25 scenes** in the simulator and the firmware; the firmware compiles for Teensy 4.1
  (Teensyduino 1.62, SmartMatrix 4 + patch) with ~196 KB of fast RAM free.
- **Shared shell** (`scenes/shell.h`): boot splash, launcher, settings, transitions with a pinned
  HUD, toasts, autoplay, night dimming — identical on desktop and device.
- **Guided fitness**: 5 yoga flows with mirrored left/right sides, 4 exercise programs (bodyweight,
  kettlebell, Tabata, core), 3-2-1 countdowns, next-up badges, halfway/tick cues, rest previews,
  completion cards.
- **Device backends**: RTC in UTC with zone + automatic EU summer time (set from Settings → Clock
  or serial), versioned EEPROM, NEC/Apple IR decoder with hold detection, DFPlayer driver with a
  command queue and advert-mode one-shots, panel test pattern.
- **Tests**: `coriolis_tests` (time zones, color math, navigation routing, workout audio contract);
  `--shots` gallery and `--poses` sheets for visual checks.
- **Not yet**: GIFs from SD on the device, weather feed (ESP32), ambient light sensor,
  personal calendar events from SD. All hardware-dependent; verified only in simulation so far.
