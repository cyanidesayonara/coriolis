// Persistent settings. The struct is the contract; where it's stored is the
// backend's business (a text file in the sim, SD/EEPROM on the device).
#ifndef CORIOLIS_SETTINGS_H
#define CORIOLIS_SETTINGS_H

#include <stdint.h>

namespace coriolis {

struct Settings {
  uint8_t brightness = 255;      // output brightness, 16..255
  int paletteIndex = 0;
  uint8_t rotation = 0;          // quarter turns
  bool autoplay = false;         // cycle scenes automatically
  uint16_t autoplaySeconds = 30;
};

class SettingsStore {
 public:
  virtual ~SettingsStore() {}
  virtual bool load(Settings& out) = 0;
  virtual void save(const Settings& s) = 0;
};

}

#endif
