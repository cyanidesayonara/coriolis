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
  uint8_t transitionMode = 0;    // 0 auto (slide on arrows, fade on jump), 1 slide, 2 fade, 3 cut
  uint8_t volume = 20;           // 0 mute .. 30 (DFPlayer scale)

  // time zone for the device RTC, which keeps UTC (the sim uses the PC's
  // local time and ignores these)
  int8_t zoneHours = 1;          // UTC offset: +1 = CET (Barcelona)
  bool euDst = true;             // EU summer time on the last Sundays of Mar/Oct

  // clock overlay, shown on top of any scene (its own button toggles type)
  uint8_t overlayType = 0;       // 0 off, 1 digital, 2 analog, 3 word
  uint8_t overlayPos = 4;        // 3x3 grid, 0=top-left .. 4=center .. 8=bot-right
  uint8_t overlaySize = 1;       // 0 small, 1 medium, 2 large

  // per-scene
  uint8_t yogaBody = 0;          // 0 female, 1 male
  uint8_t yogaProgram = 0;       // 0 wake-up, 1 sun salutation, 2 standing, 3 hips, 4 bedtime
  uint16_t yogaHoldSec = 15;
  uint8_t exerciseProgram = 0;   // 0 bodyweight, 1 kettlebell, 2 tabata, 3 core
  uint8_t exerciseReps = 8;
  uint8_t exerciseRepSec = 3;    // seconds per rep
  uint8_t breatheStyle = 0;      // 0 box (4-4-4-4), 1 relax (4-7-8)
  uint8_t breatheSec = 4;        // in-breath seconds
  uint8_t pongLevel = 1;         // 0 easy, 1 normal, 2 hard
  uint8_t snakeSpeed = 1;        // 0 slow, 1 normal, 2 fast
  bool fireSparks = true;

  // night mode schedule
  bool nightMode = true;         // auto-dim during sleep hours
  uint8_t nightStartHour = 23;   // 23:00 (11 PM)
  uint8_t nightEndHour = 7;      // 07:00 (7 AM)
  uint8_t nightBrightness = 16;  // lowest comfortable level

  // high scores
  uint16_t snakeHigh = 0;
  uint32_t tetrisHigh = 0;

  bool isNightTime(int currentHour) const {
    if (!nightMode) return false;
    if (nightStartHour > nightEndHour) {
      return currentHour >= nightStartHour || currentHour < nightEndHour;
    } else {
      return currentHour >= nightStartHour && currentHour < nightEndHour;
    }
  }

  // pull every field back into its legal range — stored settings may come
  // from a hand-edited file or an older layout, and several values are
  // divisors or table indices
  void sanitize() {
    if (brightness < 16) brightness = 16;
    if (paletteIndex < 0 || paletteIndex > 6) paletteIndex = 0;
    rotation &= 3;
    if (autoplaySeconds < 10 || autoplaySeconds > 600) autoplaySeconds = 30;
    if (transitionMode > 3) transitionMode = 0;
    if (volume > 30) volume = 20;
    if (zoneHours < -12 || zoneHours > 14) zoneHours = 1;
    if (overlayType > 3) overlayType = 0;
    if (overlayPos > 8) overlayPos = 4;
    if (overlaySize > 2) overlaySize = 1;
    yogaBody &= 1;
    if (yogaProgram > 4) yogaProgram = 0;
    if (yogaHoldSec < 5 || yogaHoldSec > 60) yogaHoldSec = 15;
    exerciseProgram &= 3;
    if (exerciseReps < 4 || exerciseReps > 20) exerciseReps = 8;
    if (exerciseRepSec < 2 || exerciseRepSec > 6) exerciseRepSec = 3;
    breatheStyle &= 1;
    if (breatheSec < 3 || breatheSec > 8) breatheSec = 4;
    if (pongLevel > 2) pongLevel = 1;
    if (snakeSpeed > 2) snakeSpeed = 1;
    nightStartHour %= 24;
    nightEndHour %= 24;
    if (nightBrightness < 8 || nightBrightness > 128) nightBrightness = 16;
  }

  uint8_t effectiveBrightness(int currentHour) const {
    if (isNightTime(currentHour)) {
      return nightBrightness < brightness ? nightBrightness : brightness;
    }
    return brightness;
  }
};

class SettingsStore {
 public:
  virtual ~SettingsStore() {}
  virtual bool load(Settings& out) = 0;
  virtual void save(const Settings& s) = 0;
};

}

#endif
