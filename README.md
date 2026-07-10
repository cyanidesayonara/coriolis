# Coriolis

LED matrix wall display — successor to [Borealis](https://github.com/cyanidesayonara/borealis)
(Aurora → Borealis → Coriolis). A wall clock first, with patterns, weather,
audio-reactive visuals and games to follow.

See [PLAN.md](PLAN.md) for the hardware plan and the platform decision.

## Architecture

Scenes (clock, patterns, games) draw into a plain framebuffer through a small
core API and never touch hardware. Backends copy that framebuffer to a real
output:

```
core/     framebuffer, color, 8-bit wave math, palettes, bitmap font, Scene API
scenes/   everything visible: clock, spiro, fire, plasma, ...
sim/      desktop backend: raylib window, system clock, keyboard input
firmware/ (later) hardware backend for the chosen platform
```

The simulator is not a side tool — it's the primary development environment.
Scenes are written and tuned on the desktop; the hardware backend only
replaces the window with panels and the keyboard with a remote/gamepad.

The display shape is a build flag (`128x64` widescreen by default, `-DSQUARE=ON`
for `128x128`), and scenes must handle both.

## Build and run the simulator

Prerequisites: CMake ≥ 3.16, a C++ compiler, git (raylib is fetched
automatically on first configure).

```sh
cmake -B build
cmake --build build
./build/coriolis_sim          # Windows: build\Debug\coriolis_sim.exe

# the 128x128 variant:
cmake -B build-square -DSQUARE=ON
cmake --build build-square
```

## Controls (simulator)

| Key | Action |
|-----|--------|
| `SPACE` / `→` | next scene |
| `←` | previous scene |
| `↑` / `↓` | cycle palette |
| `ESC` | quit |

Arrow keys are offered to the active scene first (games will consume them),
matching how a remote/gamepad will behave on the device.

## Adding a scene

1. Create `scenes/scene_yourthing.h` implementing `coriolis::Scene`
   (`name()`, `draw(Context&)`, optionally `start/stop/input`).
2. Register an instance in the `scenes[]` array in `sim/main.cpp`.
3. `cmake --build build` and it's in the rotation.

`Context` hands a scene everything it may use: the framebuffer, the time
source, the active palette, and monotonic milliseconds. If a scene needs
something new (audio frames, weather data), extend `Context` with an
interface the backends can fake — never reach around it.
