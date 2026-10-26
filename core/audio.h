// Audio is an interface, like time and input: scenes fire named cues, and a
// backend decides how to make them. On the device a DFPlayer Mini plays
// numbered MP3 clips from a microSD card (keeping the display fully offline);
// in the simulator short synthesized beeps stand in. See docs/AUDIO.md for
// the cue -> clip-number mapping.
#ifndef CORIOLIS_AUDIO_H
#define CORIOLIS_AUDIO_H

#include <stdint.h>

namespace coriolis {

enum class Cue {
  None,
  // guided activities
  Chime,       // pose / step / rep change
  StartBell,   // a session begins
  FinishBell,  // a session ends
  Halfway,     // halfway through a pose hold or set
  Tick,        // 3-2-1 countdown tick
  BreatheIn,
  BreatheHold,
  BreatheOut,
  // fireplace
  Pop,         // a spark pops (one-shot over the crackle bed)
  Crackle,     // looping ambient bed
  // games
  Eat,
  Die,
  Bounce,
  Score,
};

class AudioSink {
 public:
  virtual ~AudioSink() {}

  // fire a one-shot cue
  virtual void play(Cue) {}

  // a spoken clip by index — pose names, rep counts, etc. (device: a folder
  // of numbered voice clips; sim: a neutral blip)
  virtual void voice(int) {}

  // start/replace a looping ambient bed; Cue::None stops it
  virtual void loop(Cue) {}

  // master volume, 0 (mute) .. 30 — the DFPlayer's native range
  virtual void setVolume(uint8_t) {}

  // called once per main-loop pass, for backends that sequence clips
  // (a voice queued behind a chime) or poll a serial device
  virtual void update(uint32_t) {}
};

// spoken-clip numbers: voice(n) plays file 02/nnn.mp3 on the SD card, so
// the index IS the file number (1..255, the DFPlayer's per-folder limit).
// tools/generate_voice_pack.py writes exactly these files.
namespace voice {
static const int POSE_BASE = 1;      // yoga poses: 1 + pose id (1..19)
static const int MOVE_BASE = 100;    // exercise moves: 100 + move id (101..)
static const int NUMBER_BASE = 200;  // "one".."thirty": 200 + n
static const int SWITCH_SIDES = 241;
static const int HALFWAY = 242;
static const int COMPLETE = 243;     // "workout complete, great job"
static const int GET_READY = 244;
static const int NAMASTE = 245;
static const int WORK = 246;
static const int REST = 247;
}

// a backend that makes no sound (default, and the safe fallback)
class SilentAudio : public AudioSink {};

}

#endif
