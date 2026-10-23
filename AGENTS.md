# AGENTS.md — Coriolis

This file provides context and instructions for AI coding agents working on this project.
It follows the AGENTS.md open standard (https://agents.md).

## Project Overview

**Coriolis** is an LED matrix wall display — successor to Borealis (Aurora → Borealis → Coriolis).
A 128×128 HUB75 RGB LED matrix display driven by a Teensy 4.1 + SmartLED Shield V5.
It serves as a 24/7 living room clock first, with ambient generative art, retro games,
and a dedicated glanceable guided fitness suite (Yoga, Exercise/Kettlebell, Breathing, Focus).

Developed entirely against a desktop simulator; all scenes are written and tuned on the
desktop before flashing to hardware.

Canonical documents:
- `PROJECT.md`: durable architecture, technical reference, BOM, fitness suite design, and roadmap.
- `CHANGELOG.md`: newest-first session history and release notes.
- `README.md`: user guide, controls, and visual gallery.

## Tech Stack

- **C++11** across all shared code (`core/` and `scenes/`)
- **Simulator**: Desktop backend (`sim/main.cpp`) built with CMake and raylib 5.5
- **Firmware**: Teensy 4.1 sketch (`firmware/coriolis/coriolis.ino`) using SmartMatrix 4 + SmartLED Shield V5
- **Display Target**: 128×128 square display (four 64×64 P4/P5 HUB75 panels); 128×64 widescreen supported via `-DWIDE=ON`
- **Audio**: Interface-based (`core/audio.h`); DFPlayer Mini MP3 hardware module on device, real-time synthesized beeps in the simulator
- **Input**: Hardware serial/bench input + NEC IR remote; keyboard in the simulator

## Project Structure

```
coriolis/
  core/           # Platform-agnostic core engine (framebuffer, 8-bit math, palettes, audio, settings, fonts)
  scenes/         # 25+ visual scenes: clocks, guided fitness suite, retro games, ambient generative art
  sim/            # Desktop simulator backend (raylib window, system clock, keyboard input)
  firmware/       # Hardware firmware for Teensy 4.1 (SmartMatrix allocation, RTC time, EEPROM storage)
  docs/           # Screenshots, guides, and hardware documentation
  CMakeLists.txt  # CMake build definition for the desktop simulator
  PROJECT.md      # Durable design pillars, fitness architecture, hardware BOM, and roadmap
  CHANGELOG.md    # Session history and updates
  README.md       # Project overview, gallery, and control mappings
```

## Developer Workflow & Verification

1. **Simulator Build (MSVC on Windows)**:
   ```cmd
   cmake -B build
   cmake --build build --config Release
   .\build\Release\coriolis_sim.exe
   ```

2. **Screenshot Regeneration**:
   Verify visual regression by running:
   ```cmd
   .\build\Release\coriolis_sim.exe --shots docs/screenshots
   ```

3. **Coding Standards**:
   - Zero dynamic allocation inside per-frame scene loops (`draw()` calls must be deterministic and heap-free).
   - Keep scenes platform-agnostic: scenes draw strictly into `FrameBuffer` through `Context& ctx` and never touch hardware directly.
   - Maintain documentation integrity: preserve docstrings and architectural comments across edits.
