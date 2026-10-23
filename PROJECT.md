# PROJECT.md — Coriolis

Successor to Borealis (Aurora → Borealis → Coriolis). An all-new LED matrix
wall display: new hardware, new software, none of the 64KB-era constraints.
Borealis remains on the wall as a finished product.

Planning started July 2026. Hardware build planned for late 2026 after house move.

## Architecture & Design Pillars

- **Display: 128×128 square, chunky pixels (Decided July 2026)**.
  Four 64×64 HUB75 panels at P4 (~51×51 cm) or P5 (~64×64 cm). Square shows every
  content shape well (landscape, portrait, square) and makes software rotation lossless.
  The 128×64 widescreen build remains as a sim experiment flag (`-DWIDE=ON`);
  code stays resolution-agnostic regardless.
- **Hardware Platform: Teensy 4.1 + SmartLED Shield V5**.
  - Refresh depth: 36/48-bit color, ~120 Hz @ 128×128. Smooth gradients at minimum brightness.
  - Built-in hardware RTC (persists through power cycles with a coin cell).
  - USB Host port on Teensy 4.1 for direct wireless gamepad dongles.
  - DFPlayer Mini MP3 serial module with own microSD for offline audio (chimes, coach voices).
  - ESP32 mini UART co-processor for weather feeds (keeps display firmware completely offline).
- **Physical IR Remote Control**:
  - Upgrading to a premium minimalist bead-blasted aluminum unibody NEC IR remote (Apple A1294 style).
  - Crisp, weighted tactile buttons: D-Pad (Up/Down/Left/Right), Center/OK, Back/Home, Play/Pause.
  - Mat-friendly ergonomics for fitness: pause workout, skip/redo exercise, adjust hold duration on the fly.
- **Desktop-First Development**:
  - The desktop simulator (`sim/main.cpp` using raylib) is the primary development environment.
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

## Draft BOM (Teensy 4.1 Variant, 128×128)

| Item | Est. | Notes |
|---|---|---|
| Teensy 4.1 | ~$31 | SparkFun |
| SmartLED Shield for Teensy 4 (x2 — spare) | ~$24 ea | Smart-Prototyping / Crowd Supply / Jameco |
| 4x 64×64 P4 (or P5) HUB75 panels | ~€30-45 ea | Same scan type, matched batch |
| 5V PSU, 40A | ~€35-45 | Mean Well LRS-200-5 class |
| DFPlayer Mini + small speaker | ~€6 | Audio cues, chimes, crackle bed |
| Minimalist Aluminum IR Remote + Receiver | ~€15-20 | Apple A1294 style, NEC protocol |
| 8MB PSRAM chip for Teensy 4.1 | ~€3 | Solder-on; GIFs, DOOM |
| ESP32 mini (UART weather feeder) | ~€5 | Offline serial feeder |
| Light sensor (TSL2591 / photoresistor) | ~€2-5 | Auto-dimming |
| Coin cell for RTC, wiring, frame extrusion | ~€25-35 | 2020 aluminum extrusion |

Rough total: ~€280–330.

## Current Software Status

- **25 scenes running in simulator and firmware** with 100% test coverage via `--shots docs/screenshots`.
- **Guided Fitness Suite**: Bilateral flows, 5 yoga routines, 4 exercise programs, halfway & countdown chimes, next-pose HUD previews, completion cards.
- **Scene Launcher**: Direct categorized switcher (`M` key).
- **Firmware sync**: `CoriolisShared.h` umbrella header fully synced with all 25 scenes.
