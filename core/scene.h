// A Scene is anything that owns the display for a while: the clock, a
// pattern, a game. The evolution of Borealis's Drawable — same spirit, but
// scenes receive time and input explicitly instead of reaching for globals,
// which is what makes them run identically on desktop and hardware.
#ifndef CORIOLIS_SCENE_H
#define CORIOLIS_SCENE_H

#include <stdint.h>

#include "display.h"
#include "palette.h"
#include "audio.h"

namespace coriolis {

struct TimeOfDay {
  int hour;    // 0-23
  int minute;  // 0-59
  int second;  // 0-59
};

// backends provide real time (system clock / RTC); scenes just ask
class TimeSource {
 public:
  virtual ~TimeSource() {}
  virtual TimeOfDay now() = 0;
  virtual bool available() { return true; }
};

enum class Key {
  None,
  Up,
  Down,
  Left,
  Right,
  Select,
  Back,
};

// continuous input for games — "is this key held right now?" Backends map
// it to whatever they have (keyboard in the sim, gamepad on the device).
class InputState {
 public:
  virtual ~InputState() {}
  virtual bool isDown(Key) const = 0;
};

// everything a scene may touch, handed in each frame
struct Context {
  FrameBuffer& fb;
  TimeSource& time;
  InputState& held;
  AudioSink& audio;
  const Palette* palette;
  uint32_t nowMs;  // monotonic milliseconds since start
};

class Scene {
 public:
  virtual ~Scene() {}
  virtual const char* name() const = 0;

  // autoplay skips scenes that shouldn't be interrupted or cycled into
  // (settings, an activity in progress, a game being played)
  virtual bool autoplayEligible() const { return true; }

  virtual void start(Context&) {}
  virtual void stop(Context&) {}

  // draw one frame; return the requested delay in ms before the next one
  virtual uint32_t draw(Context&) = 0;

  // returns true if the key was consumed (games take arrows, patterns don't)
  virtual bool input(Context&, Key) { return false; }
};

}

#endif
