# CHANGELOG — Coriolis

All notable changes to Coriolis will be documented in this file, newest first.

## [Unreleased] - 2026-10-23

### Added
- **DFPlayer Mini Voice Pack Generator**:
  - Created `tools/generate_voice_pack.py` to synthesize the complete offline audio suite (sound cues, yoga pose names, exercise movements, spoken counts 1..30, and guidance prompts) for the hardware DFPlayer Mini MP3 module.
  - Supports Microsoft Edge Neural TTS (`en-US-AriaNeural`) with an offline Windows SAPI fallback, alongside pure mathematical 16-bit 44.1kHz PCM synthesis for chimes, bells, and ticks.
  - Updated `docs/AUDIO.md` with complete track index tables, FAT32 microSD folder layout (`01/` and `02/`), and Teensy 4.1 hardware wiring schematics.
- **Smooth Scene-to-Scene Transition Engine**:
  - Implemented hardware-agnostic `TransitionEngine` in `core/transition.h` supporting directional horizontal slides (`SlideLeft`, `SlideRight`), vertical slides (`SlideUp`, `SlideDown`), smooth crossfades (`CrossFade`), and curtain wipes (`WipeLeft`, `WipeRight`) with zero per-frame heap allocations.
  - Smoothstep easing ($3t^2 - 2t^3$) ensures physical, buttery acceleration and deceleration across all transitions.
  - Next/Previous arrow navigation automatically triggers physical slide transitions (`SlideLeft` on next, `SlideRight` on prev), while direct launcher selection and home jumps trigger smooth cross-dissolves.
  - Stationary HUD compositing ensures action confirmations (Toasts) and clock overlays remain crisply pinned while scenes slide smoothly underneath.
  - Added configurable `TRANSIT` option in `GENERAL` settings (`AUTO`, `SLIDE`, `FADE`, `CUT`) persisted across sessions.
  - Synchronized transition pipeline across both desktop simulator (`sim/main.cpp`) and Teensy 4.1 hardware firmware (`firmware/coriolis/coriolis.ino`).
- **Night Mode & Sleep Auto-Dimming**:
  - Added schedule-based automatic dimming in `core/settings.h` (`nightMode`, `nightStartHour`, `nightEndHour`, `nightBrightness`).
  - Added dedicated `NIGHT` section to `scenes/scene_settings.h` allowing custom start/end hour and sleep brightness configuration.
  - Applied effective brightness automatically in both desktop simulator and Teensy 4.1 hardware firmware.
- **Hardware NEC & Apple IR Remote Driver**:
  - Added interrupt-driven 38kHz NEC pulse decoder in `firmware/coriolis/coriolis.ino` on Pin 2 supporting Apple A1294 aluminum remote and standard universal NEC remotes.
  - Added `docs/REMOTE.md` guide with full wiring schematics, TSOP38238 pinout, command codes, and bench bring-up steps.
- **Bilateral Flow Engine for Fitness & Yoga**:
  - Implemented `guide::mirrorPose()` in `scenes/figure.h` with automatic vertical center inversion and anatomical left/right limb swapping.
  - Added bilateral pose support to `scenes/scene_yoga.h` with clear `(R)` and `(L)` side banners and side-switching transitions.
  - Added bilateral exercise support in `scenes/scene_exercise.h` for lunges, kettlebell rows, and overhead presses.
- **Expanded Routine Library**:
  - 5 complete Yoga flows: *Morning Wake-Up* (spine & stretch), *Sun Salutation* (vinyasa loop), *Standing Strength & Balance* (bilateral warriors & tree), *Hips & Back* (desk worker relief), and *Bedtime Restorative* (calming floor holds).
  - 4 complete Exercise programs: *Bodyweight Circuit*, *Kettlebell Full-Body*, *HIIT Tabata* (20s on / 10s rest with full-screen color-coded cues), and *Core & Pilates*.
  - Added 5 new yoga poses: `CAT`, `COW`, `LOW_LUNGE`, `PIGEON`, and `BOAT`.
- **HUD Previews & Audio Guidance**:
  - Added "Next Up" preview badge (`guide::drawNextBadge`) appearing 4 seconds before pose or interval transitions so users can prepare.
  - Added `Cue::Halfway` chime (notifying you when 50% of a pose hold is complete) and `Cue::Tick` countdown audio cues in `core/audio.h` and synthesized in `sim/main.cpp`.
  - Added 3-2-1 countdown screen (`guide::drawCountdown`) before workouts begin.
  - Added celebratory completion card (`guide::drawCompletion`) with workout duration, rings, and celebratory chime when finishing a flow.
- **Categorized Scene Launcher Overlay**:
  - Added `scenes/scene_launcher.h` (`M` key in simulator, remote menu on hardware) with 4 clear categories: *Clocks & Info*, *Guided Fitness*, *Games*, and *Ambient Art*, allowing direct jumping across all 25 scenes.
- **Project Structure & Governance**:
  - Added `AGENTS.md` and `PROJECT.md` adhering to the project structure standards established in adjacent projects (`path-of-leash-resistance` and `stemma`).
  - Synced `firmware/setup.ps1` to include all missing scenes (`scene_weather.h`, `scene_focus.h`, `scene_labyrinth.h`, `scene_lava.h`, `scene_launcher.h`).
