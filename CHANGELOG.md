# CHANGELOG — Coriolis

All notable changes to Coriolis will be documented in this file, newest first.

## [Unreleased] - 2026-11-23 — navigation escapes, figure overhaul, and visual polish

### Fixed
- **Labyrinth & game navigation traps**: Pressing `Back` (`Backspace` / `Esc`) inside Labyrinth, Pong, Snake, Tetris, Breathe, or Focus now returns cleanly to the scene's setup card where `Left`/`Right` immediately switch scenes; `OK` (`Enter`) in Labyrinth or Pong also opens a pause menu with an `Exit` option for remotes without a Back button.
- **Simulator `ESC` behavior**: Pressing `Esc` in the desktop simulator now backs out of active scenes/menus (like `Backspace`) instead of abruptly terminating the window, and exits cleanly when back on the home Clock.
- **Focus & Countdown layout**: Fixed the `"FOCUS"` title colliding with the draining timer ring and gave the 3-2-1 `"Get ready"` label extra clearance above the countdown arc.
- **Supine pose head orientation**: Supine poses (`Bridge`, `Savasana`, `Situp`, `Glute Bridge`) now face upward toward the ceiling instead of into the mat.

### Changed / Improved
- **Athletic figure & animation overhaul (`scenes/figure.h`)**: Replaced blocky stick/slab geometry with anti-aliased tapered shaded capsules, oriented oval heads with distinct female/male outfits and hair, 3-layer near/far limb depth sorting, mat highlights with contact shadows, curved spine support (`Cat`, `Cow`, `Cobra`, `Child's Pose`, `Forward Fold`, `Bridge`, `Boat`, `Situp`), an upgraded kettlebell, and arc-preserving kinematic pose interpolation (`tweenPoseKinematic`) with 2-bone IK for planted ankles.
- **Refined fitness poses & cues (`scenes/scene_yoga.h`, `scenes/scene_exercise.h`)**: Switched bodyweight Squat to a clear side profile, gave Jumping Jacks and Kettlebell Swings true sweeping pendulum arcs (with a motion trail behind the kettlebell), aligned raised arms with the ear line in Warrior 1 / Low Lunge, and sharpened High Knees and Mountain Climbers.
- **Display & UI polish**: Added SDF anti-aliased `drawArc`, `fillCircleAA`, and `blend` primitives (`core/display.h`); upgraded Breathe with layered phase-colored radial auras and an outer progress arc; upgraded Focus, Weather, Calendar, and Pong to `font5x7` rounded numerals/arcs; added procedural stone seams, trim, maze size selection (`13×13` / `19×19` / `25×25`), and fog-of-war minimap shading to Labyrinth; added 3D block bevels to Tetris, gradient shading + eyes + pulsing apple to Snake, ball trail to Pong, 60-minute dial dots + counterweight hand to Analog Clock, and shaded oval fish with wagging tails, kelp, and pebbles to Aquarium.
- **Performance, hardware safety & manuals**: Eliminated per-pixel divisions in `LavaScene` and `AuroraScene`; added debounced auto-persistence in `Shell` so game high scores and in-session tweaks save to EEPROM without entering Settings; added a dynamic frame current limiter (`powerLimitedBrightness`) in `core/display.h` and `firmware/coriolis/coriolis.ino`; updated `docs/manual/user_manual.html` and regenerated `docs/Coriolis_User_Manual.pdf` and `docs/Coriolis_Build_Guide.pdf`.

## [Unreleased] - 2026-10-26 — review, fixes and polish before the hardware build

### Fixed (review of the October 23 changes)
- **Firmware did not compile**: duplicate `case 0x07` in the IR decoder. Verified the whole
  firmware now builds with arduino-cli + Teensyduino 1.62.
- **IR receiver was on pin 2**, which is the SmartLED Shield V5's panel OE line. Moved to pin 16.
  The decoder read Apple commands from the wrong byte, never mapped Settings/Back, and ignored
  repeat frames; rewritten with validation, repeats (held keys) and tap/hold for Menu and Play.
- **Features documented but never implemented** on October 23 (bilateral flows, `mirrorPose`,
  five yoga flows, four exercise programs, Tabata, next-up badges, countdown and completion
  screens, halfway/tick cues). They are implemented now; settings showed five yoga flow names
  while only three existed.
- **Launcher** received no arrow keys while a game was running (the game took them first);
  scene indices were hard-coded; `&` and `/` didn't exist in the font.
- **`lerp8` overflowed when fading down** (255→195 at 50% gave 97), corrupting every palette
  blend, the crossfade transition and gradients. Fixed; gradient palettes no longer wrap their
  top end back to black.
- **EEPROM layout** changed without a version bump (old settings would load as garbage). Now
  stores magic + struct size + build stamp and falls back to defaults on mismatch.
- **Audio numbering**: exercise names were spoken by program position (kettlebell swings said
  "squats"), rep counts were off by one, and prompt clips sat above the DFPlayer's 255-file folder
  limit. Clip index = file number now (`core/audio.h`, `namespace voice`).
- The clock overlay and toast were drawn into the scene's own canvas, burning into trail-style
  scenes; they are composited on a separate output now and stay pinned during slides.
- Second-pass review: on the device a move's spoken name was replaced by "one" (DFPlayer
  now queues voice clips), "get ready" was cut off by the first countdown tick, choosing the
  current scene in the launcher restarted it, the sim didn't save volume/zone/summer time,
  timers weren't safe across the 49-day millis() wrap, loaded settings weren't range-checked,
  and re-flashing could shift a correct RTC by the UTC offset.
- Removed `docs/CORIOLIS_HARDWARE_MANUAL.pdf` and its generator (pin 2, wrong frame sizes);
  replaced by `docs/BUILD.md` and `docs/Coriolis_Build_Guide.pdf`.

### Added
- `scenes/shell.h`: one navigation/menu/transition implementation for both backends.
- DFPlayer Mini driver (Serial1), RTC in UTC with time zone + automatic EU summer time,
  CLOCK settings section (set time/date/zone on the device), volume setting, serial time sync,
  panel-mapping test pattern, optional brightness cap and panel-chain options.
- UI facelift: 5×7 proportional font with lowercase and smooth scaling (`core/font5x7.h`), new
  clock face (rounded gradient digits, breathing colon, date, seconds bar), icon-tab launcher with
  animated selection, sliding toasts, redesigned setup cards, boot splash.
- Yoga: cat, cow, low lunge, pigeon, boat poses. Exercise: glute bridge, plank hold, mountain
  climbers, high knees.
- `tests/test_core.cpp` (`coriolis_tests`), `--poses` render mode, new gallery shots.
- `docs/Coriolis_User_Manual.pdf` (consumer-style user's manual) and
  `docs/Coriolis_Build_Guide.pdf`, rendered from `docs/manual/*.html` by `tools/render_manuals.py`.

## [2026-10-23] (notes below were partly inaccurate — see the review above)



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
