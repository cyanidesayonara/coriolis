# One-time firmware toolchain setup (Windows). Re-runnable.
# Prereqs: git; arduino-cli (winget install ArduinoSA.CLI).
#
# Installs the Teensy platform, clones SmartMatrix 4 (with the small GCC11
# compatibility patch), and creates the CoriolisShared library that exposes
# the repo's core/ and scenes/ to the sketch via directory junctions.
#
# Then: arduino-cli compile --fqbn teensy:avr:teensy41 firmware/coriolis

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$cli = "C:\Program Files\Arduino CLI\arduino-cli.exe"
if (-not (Test-Path $cli)) { $cli = "arduino-cli" }  # hope it's on PATH

# project-local libraries so nothing collides with other Arduino installs
& $cli config set directories.user "$repo\firmware\arduino"
& $cli config add board_manager.additional_urls "https://www.pjrc.com/teensy/package_teensy_index.json"
& $cli core update-index
& $cli core install teensy:avr

$libs = "$repo\firmware\arduino\libraries"
New-Item -ItemType Directory -Force -Path $libs | Out-Null

# SmartMatrix 4 (master) + the GCC11 section/alias fixes (see the .patch)
if (-not (Test-Path "$libs\SmartMatrix")) {
  git clone --recursive https://github.com/pixelmatix/SmartMatrix.git "$libs\SmartMatrix"
  git -C "$libs\SmartMatrix" apply "$repo\firmware\smartmatrix-gcc11.patch"
}

# CoriolisShared: a thin library whose src/ junctions into the real repo code
$shared = "$libs\CoriolisShared"
New-Item -ItemType Directory -Force -Path "$shared\src" | Out-Null
@"
name=CoriolisShared
version=1.0.0
author=cyanidesayonara
maintainer=cyanidesayonara
sentence=Coriolis shared core and scenes (junctioned from the repo).
paragraph=Do not edit here; the real files live in the repo's core/ and scenes/.
category=Display
url=https://github.com/cyanidesayonara/coriolis
architectures=*
"@ | Set-Content "$shared\library.properties"
if (-not (Test-Path "$shared\src\core")) {
  New-Item -ItemType Junction -Path "$shared\src\core" -Target "$repo\core" | Out-Null
}
if (-not (Test-Path "$shared\src\scenes")) {
  New-Item -ItemType Junction -Path "$shared\src\scenes" -Target "$repo\scenes" | Out-Null
}
@"
// Umbrella header for the shared Coriolis code. Including this from the
// sketch activates the library, which puts src/ (with its core/ and scenes/
// junctions into the real repo) on the include path.
#ifndef CORIOLIS_SHARED_H
#define CORIOLIS_SHARED_H

#include "core/config.h"
#include "core/display.h"
#include "core/scene.h"
#include "core/settings.h"
#include "core/events.h"
#include "scenes/scene_clock.h"
#include "scenes/scene_analogclock.h"
#include "scenes/scene_wordclock.h"
#include "scenes/scene_calendar.h"
#include "scenes/scene_pong.h"
#include "scenes/scene_snake.h"
#include "scenes/scene_tetris.h"
#include "scenes/scene_yoga.h"
#include "scenes/scene_exercise.h"
#include "scenes/scene_breathe.h"
#include "scenes/scene_gifs.h"
#include "scenes/scene_spiro.h"
#include "scenes/scene_mandala.h"
#include "scenes/scene_rain.h"
#include "scenes/scene_bounce.h"
#include "scenes/scene_starfield.h"
#include "scenes/scene_life.h"
#include "scenes/scene_aquarium.h"
#include "scenes/scene_fire.h"
#include "scenes/scene_aurora.h"
#include "scenes/scene_coriolis.h"
#include "scenes/scene_settings.h"
#include "scenes/clock_overlay.h"
#include "scenes/toast.h"

#endif
"@ | Set-Content "$shared\src\CoriolisShared.h"

Write-Output "Firmware toolchain ready. Build with:"
Write-Output "  arduino-cli compile --fqbn teensy:avr:teensy41 firmware/coriolis"
