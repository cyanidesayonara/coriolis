// Persistent settings. The struct is the contract; where it's stored is the
// backend's business (a text file in the sim, SD/EEPROM on the device).
#ifndef CORIOLIS_SETTINGS_H
#define CORIOLIS_SETTINGS_H

#include <stdint.h>

namespace coriolis {

struct Settings {
  // general
  uint8_t brightness = 255;      // output brightness, 16..255
  int paletteIndex = 0;
  uint8_t rotation = 0;          // quarter turns
  bool autoplay = false;         // cycle scenes automatically
  uint16_t autoplaySeconds = 30;

  // clock overlay, shown on top of any scene (its own button toggles type)
  uint8_t overlayType = 0;       // 0 off, 1 digital, 2 analog, 3 word
  uint8_t overlayPos = 4;        // 3x3 grid, 0=top-left .. 4=center .. 8=bot-right
  uint8_t overlaySize = 1;       // 0 small, 1 medium, 2 large

  // per-scene
  uint8_t yogaBody = 0;          // 0 female, 1 male
  uint16_t yogaHoldSec = 15;
  uint8_t exerciseProgram = 0;   // 0 bodyweight, 1 kettlebell
  uint8_t exerciseReps = 8;
  uint8_t exerciseRepSec = 3;    // seconds per rep
  uint8_t breatheStyle = 0;      // 0 box (4-4-4-4), 1 relax (4-7-8)
  uint8_t breatheSec = 4;        // in-breath seconds
  uint8_t pongLevel = 1;         // 0 easy, 1 normal, 2 hard
  uint8_t snakeSpeed = 1;        // 0 slow, 1 normal, 2 fast
  bool fireSparks = true;
};

class SettingsStore {
 public:
  virtual ~SettingsStore() {}
  virtual bool load(Settings& out) = 0;
  virtual void save(const Settings& s) = 0;
};

}

#endif
