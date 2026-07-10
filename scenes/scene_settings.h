// Settings scene: up/down picks a row, left/right adjusts it. Values apply
// live (the backend reads the Settings struct every frame) and persist when
// the scene is left.
#ifndef CORIOLIS_SCENE_SETTINGS_H
#define CORIOLIS_SCENE_SETTINGS_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/settings.h"
#include "../core/palette.h"

namespace coriolis {

class SettingsScene : public Scene {
 public:
  SettingsScene(Settings& settings, SettingsStore& store)
      : settings_(settings), store_(store) {}

  const char* name() const { return "Settings"; }
  bool autoplayEligible() const { return false; }

  void start(Context&) { row_ = 0; dirty_ = false; }

  void stop(Context&) {
    if (dirty_) store_.save(settings_);
    dirty_ = false;
  }

  bool input(Context&, Key k) {
    if (k == Key::Up) { row_ = (row_ + ROWS - 1) % ROWS; return true; }
    if (k == Key::Down) { row_ = (row_ + 1) % ROWS; return true; }

    int dir = k == Key::Right ? 1 : (k == Key::Left ? -1 : 0);
    if (dir == 0) return false;

    dirty_ = true;
    switch (row_) {
      case 0: {  // brightness in coarse steps
        int b = settings_.brightness + dir * 32;
        if (b < 16) b = 16;
        if (b > 255) b = 255;
        settings_.brightness = uint8_t(b);
        break;
      }
      case 1:
        settings_.paletteIndex =
            (settings_.paletteIndex + dir + palettes::COUNT) % palettes::COUNT;
        break;
      case 2:
        settings_.rotation = uint8_t((settings_.rotation + dir + 4) % 4);
        break;
      case 3:
        settings_.autoplay = !settings_.autoplay;
        break;
      case 4: {
        int s = settings_.autoplaySeconds + dir * 10;
        if (s < 10) s = 10;
        if (s > 600) s = 600;
        settings_.autoplaySeconds = uint16_t(s);
        break;
      }
    }
    return true;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    RGB title = ctx.palette->lookupBright(0);
    font3x5::drawText(ctx.fb, "SETTINGS", 4, 3, 1, title);
    ctx.fb.hLine(0, ctx.fb.width() - 1, 11, RGB(50, 50, 50));

    const char* labels[ROWS] = {"BRIGHT", "PALETTE", "ROTATE", "AUTO",
                                "SECS"};
    char value[16];

    int y = 16;
    int rowH = (ctx.fb.height() - 20) / ROWS;
    if (rowH > 14) rowH = 14;

    for (int i = 0; i < ROWS; i++) {
      bool selected = i == row_;
      RGB color = selected ? ctx.palette->lookupBright(0) : RGB(120, 120, 120);

      if (selected) font3x5::drawText(ctx.fb, ">", 1, y, 1, color);
      font3x5::drawText(ctx.fb, labels[i], 7, y, 1, color);

      switch (i) {
        case 0:
          snprintf(value, sizeof(value), "%d", settings_.brightness);
          break;
        case 1:
          snprintf(value, sizeof(value), "%s",
                   palettes::byIndex(settings_.paletteIndex).name);
          break;
        case 2:
          snprintf(value, sizeof(value), "%d", settings_.rotation * 90);
          break;
        case 3:
          snprintf(value, sizeof(value), "%s",
                   settings_.autoplay ? "ON" : "OFF");
          break;
        case 4:
          snprintf(value, sizeof(value), "%d", settings_.autoplaySeconds);
          break;
      }
      // uppercase for the 3x5 font's sake
      for (char* p = value; *p; ++p)
        if (*p >= 'a' && *p <= 'z') *p = char(*p - 'a' + 'A');

      int vw = font3x5::textWidth(value, 1);
      font3x5::drawText(ctx.fb, value, ctx.fb.width() - vw - 3, y, 1, color);

      y += rowH;
    }

    return 50;
  }

 private:
  static const int ROWS = 5;

  Settings& settings_;
  SettingsStore& store_;
  int row_ = 0;
  bool dirty_ = false;
};

}

#endif
