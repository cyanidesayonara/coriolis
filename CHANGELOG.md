# CHANGELOG — Coriolis

All notable changes to Coriolis will be documented in this file, newest first.

## [Unreleased] - 2026-10-23

### Added
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
