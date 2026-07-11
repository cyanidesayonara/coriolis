// Audio is an interface, like time and input: scenes fire named cues, and a
// backend decides how to make them. On the device a DFPlayer Mini plays
// numbered MP3 clips from a microSD card (keeping the display fully offline);
// in the simulator short synthesized beeps stand in. See docs/AUDIO.md for
// the cue -> clip-number mapping.
#ifndef CORIOLIS_AUDIO_H
#define CORIOLIS_AUDIO_H

namespace coriolis {

enum class Cue {
  None,
  // guided activities
  Chime,       // pose / step / rep change
  StartBell,   // a session begins
  FinishBell,  // a session ends
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
};

// a backend that makes no sound (default, and the safe fallback)
class SilentAudio : public AudioSink {};

}

#endif
