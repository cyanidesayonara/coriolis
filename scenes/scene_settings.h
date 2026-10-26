// Settings: one scrollable list with section headers — GENERAL first, then
// a section per scene. Opened via the dedicated settings button (never part
// of the scene rotation), and opening it from a scene jumps straight to
// that scene's section. Up/down picks a row, left/right adjusts, values
// apply live and persist when the menu is left.
#ifndef CORIOLIS_SCENE_SETTINGS_H
#define CORIOLIS_SCENE_SETTINGS_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/settings.h"
#include "../core/palette.h"
#include "../core/font5x7.h"
#include "../core/timeutil.h"

namespace coriolis {

class SettingsScene : public Scene {
 public:
  SettingsScene(Settings& settings, SettingsStore& store)
      : settings_(settings), store_(store) {}

  const char* name() const { return "Settings"; }
  bool autoplayEligible() const { return false; }

  // jump to the section for the scene the user came from
  void openSection(const char* sceneName) {
    row_ = 0;
    for (int i = 0; i < ITEM_COUNT; i++) {
      if (sectionMatches(items_[i].section, sceneName)) {
        row_ = i;
        break;
      }
    }
  }

  void stop(Context&) {
    if (dirty_) store_.save(settings_);
    dirty_ = false;
  }

  bool input(Context& ctx, Key k) {
    if (k == Key::Up) { row_ = (row_ + ITEM_COUNT - 1) % ITEM_COUNT; return true; }
    if (k == Key::Down) { row_ = (row_ + 1) % ITEM_COUNT; return true; }

    int dir = k == Key::Right ? 1 : (k == Key::Left ? -1 : 0);
    if (dir == 0) return false;

    dirty_ = true;
    adjust(ctx, items_[row_].id, dir);
    return true;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    RGB titleColor = ctx.palette->lookupBright(0);
    font5x7::drawText(ctx.fb, "Settings", 4, 2, 1, titleColor);
    ctx.fb.hLine(0, ctx.fb.width() - 1, 11, RGB(50, 50, 50));

    // build the visible window: headers take a row of their own
    const int rowH = 8;
    const int top = 14;
    int maxRows = (ctx.fb.height() - top - 2) / rowH;

    // flatten items + headers into display rows, remembering where the
    // selected item lands
    int displayCount = 0;
    int selectedDisplay = 0;
    struct Row { bool header; int item; };
    Row rows[ITEM_COUNT * 2];
    const char* lastSection = "";
    for (int i = 0; i < ITEM_COUNT; i++) {
      if (strcmp(items_[i].section, lastSection) != 0) {
        rows[displayCount].header = true;
        rows[displayCount].item = i;
        displayCount++;
        lastSection = items_[i].section;
      }
      if (i == row_) selectedDisplay = displayCount;
      rows[displayCount].header = false;
      rows[displayCount].item = i;
      displayCount++;
    }

    // keep the selection inside the window
    if (selectedDisplay < scroll_) scroll_ = selectedDisplay;
    if (selectedDisplay >= scroll_ + maxRows)
      scroll_ = selectedDisplay - maxRows + 1;

    int y = top;
    for (int d = scroll_; d < displayCount && d < scroll_ + maxRows; d++) {
      const Row& r = rows[d];
      if (r.header) {
        ctx.fb.rect(1, y + 5, 2, 1, RGB(0, 120, 105));  // section tick
        font3x5::drawText(ctx.fb, items_[r.item].section, 4, y, 1,
                          RGB(0, 175, 150));
      } else {
        bool selected = r.item == row_;
        if (selected) {  // a soft highlight bar behind the active row
          RGB bar = ctx.palette->lookupBright(0);
          bar.dim(55);
          ctx.fb.rect(0, y - 1, ctx.fb.width(), rowH - 1, bar);
        }
        RGB color = selected ? RGB(255, 255, 255) : RGB(130, 130, 130);
        font3x5::drawText(ctx.fb, items_[r.item].label, 6, y, 1, color);

        char value[16];
        valueText(ctx, items_[r.item].id, value, sizeof(value));
        for (char* p = value; *p; ++p) *p = char(toupper((unsigned char)*p));
        int vw = font3x5::textWidth(value, 1);
        RGB vcol = selected ? ctx.palette->lookupBright(0) : RGB(150, 150, 150);
        font3x5::drawText(ctx.fb, value, ctx.fb.width() - vw - 4, y, 1, vcol);
      }
      y += rowH;
    }

    // scrollbar: shows how far down the list you are
    int trackH = maxRows * rowH;
    int thumbH = trackH * maxRows / displayCount;
    if (thumbH < 4) thumbH = 4;
    int thumbY = top + (trackH - thumbH) * scroll_ /
                           (displayCount - maxRows > 0 ? displayCount - maxRows : 1);
    ctx.fb.rect(ctx.fb.width() - 2, top, 1, trackH, RGB(40, 40, 40));
    ctx.fb.rect(ctx.fb.width() - 2, thumbY, 1, thumbH, RGB(0, 175, 150));

    return 50;
  }

 private:
  struct Item {
    const char* section;
    const char* label;
    uint8_t id;
  };

  static const int ITEM_COUNT = 32;
  const Item items_[ITEM_COUNT] = {
      {"GENERAL", "BRIGHT", 0},   {"GENERAL", "VOLUME", 24},
      {"GENERAL", "PALETTE", 1},
      {"GENERAL", "TRANSIT", 23}, {"GENERAL", "ROTATE", 2},
      {"GENERAL", "AUTO", 3},     {"GENERAL", "SECS", 4},
      {"CLOCK", "HOUR", 25},      {"CLOCK", "MINUTE", 26},
      {"CLOCK", "DAY", 27},       {"CLOCK", "MONTH", 28},
      {"CLOCK", "YEAR", 29},      {"CLOCK", "ZONE", 30},
      {"CLOCK", "SUMMER", 31},    {"NIGHT", "AUTO", 19},
      {"NIGHT", "START", 20},     {"NIGHT", "END", 21},
      {"NIGHT", "BRIGHT", 22},    {"OVERLAY", "CLOCK", 15},
      {"OVERLAY", "POS", 16},     {"OVERLAY", "SIZE", 17},
      {"YOGA", "FLOW", 18},       {"YOGA", "BODY", 5},
      {"YOGA", "HOLD", 6},
      {"EXERCISE", "PROGRAM", 11}, {"EXERCISE", "REPS", 12},
      {"EXERCISE", "PACE", 13},   {"BREATHE", "STYLE", 7},
      {"BREATHE", "BREATH", 8},   {"PONG", "LEVEL", 9},
      {"SNAKE", "SPEED", 14},     {"FIRE", "SPARKS", 10},
  };

  Settings& settings_;
  SettingsStore& store_;
  int row_ = 0;
  int scroll_ = 0;
  bool dirty_ = false;

  static bool sectionMatches(const char* section, const char* sceneName) {
    // case-insensitive prefix match: scene "Yoga" -> section "YOGA"
    for (int i = 0; section[i] && sceneName[i]; i++) {
      if (toupper((unsigned char)section[i]) !=
          toupper((unsigned char)sceneName[i]))
        return false;
    }
    return true;
  }

  // nudge one field of the local date/time on a settable clock (device RTC)
  static void adjustTime(Context& ctx, int field, int dir) {
    if (!ctx.time.settable()) return;
    TimeOfDay t = ctx.time.now();
    switch (field) {
      case 0: t.hour = (t.hour + dir + 24) % 24; break;
      case 1: t.minute = (t.minute + dir + 60) % 60; t.second = 0; break;
      case 2: {
        int n = timeutil::daysInMonth(t.year, t.month);
        t.day = (t.day - 1 + dir + n) % n + 1;
        break;
      }
      case 3: t.month = (t.month - 1 + dir + 12) % 12 + 1; break;
      case 4: t.year += dir; if (t.year < 2024) t.year = 2024; break;
    }
    int n = timeutil::daysInMonth(t.year, t.month);
    if (t.day > n) t.day = n;
    ctx.time.setLocal(t);
  }

  void adjust(Context& ctx, uint8_t id, int dir) {
    switch (id) {
      case 0: {
        int b = settings_.brightness + dir * 32;
        settings_.brightness = uint8_t(b < 16 ? 16 : (b > 255 ? 255 : b));
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
        settings_.autoplaySeconds = uint16_t(s < 10 ? 10 : (s > 600 ? 600 : s));
        break;
      }
      case 5:
        settings_.yogaBody = uint8_t(1 - settings_.yogaBody);
        break;
      case 6: {
        int s = settings_.yogaHoldSec + dir * 5;
        settings_.yogaHoldSec = uint16_t(s < 5 ? 5 : (s > 60 ? 60 : s));
        break;
      }
      case 7:
        settings_.breatheStyle = uint8_t(1 - settings_.breatheStyle);
        break;
      case 8: {
        int s = settings_.breatheSec + dir;
        settings_.breatheSec = uint8_t(s < 3 ? 3 : (s > 8 ? 8 : s));
        break;
      }
      case 9: {
        int l = settings_.pongLevel + dir;
        settings_.pongLevel = uint8_t(l < 0 ? 0 : (l > 2 ? 2 : l));
        break;
      }
      case 10:
        settings_.fireSparks = !settings_.fireSparks;
        break;
      case 11:
        settings_.exerciseProgram = uint8_t((settings_.exerciseProgram + dir + 4) % 4);
        break;
      case 12: {
        int r = settings_.exerciseReps + dir * 2;
        settings_.exerciseReps = uint8_t(r < 4 ? 4 : (r > 20 ? 20 : r));
        break;
      }
      case 13: {
        int p = settings_.exerciseRepSec + dir;
        settings_.exerciseRepSec = uint8_t(p < 2 ? 2 : (p > 6 ? 6 : p));
        break;
      }
      case 14: {
        int s = settings_.snakeSpeed + dir;
        settings_.snakeSpeed = uint8_t(s < 0 ? 0 : (s > 2 ? 2 : s));
        break;
      }
      case 15:
        settings_.overlayType = uint8_t((settings_.overlayType + dir + 4) % 4);
        break;
      case 16:
        settings_.overlayPos = uint8_t((settings_.overlayPos + dir + 9) % 9);
        break;
      case 17: {
        int s = settings_.overlaySize + dir;
        settings_.overlaySize = uint8_t(s < 0 ? 0 : (s > 2 ? 2 : s));
        break;
      }
      case 18:
        settings_.yogaProgram = uint8_t((settings_.yogaProgram + dir + 5) % 5);
        break;
      case 19:
        settings_.nightMode = !settings_.nightMode;
        break;
      case 20: {
        int h = (settings_.nightStartHour + dir + 24) % 24;
        settings_.nightStartHour = uint8_t(h);
        break;
      }
      case 21: {
        int h = (settings_.nightEndHour + dir + 24) % 24;
        settings_.nightEndHour = uint8_t(h);
        break;
      }
      case 22: {
        int b = settings_.nightBrightness + dir * 8;
        settings_.nightBrightness = uint8_t(b < 8 ? 8 : (b > 128 ? 128 : b));
        break;
      }
      case 23:
        settings_.transitionMode = uint8_t((settings_.transitionMode + dir + 4) % 4);
        break;
      case 24: {
        int v = settings_.volume + dir * 3;
        settings_.volume = uint8_t(v < 0 ? 0 : (v > 30 ? 30 : v));
        break;
      }
      case 25: case 26: case 27: case 28: case 29:
        adjustTime(ctx, id - 25, dir);
        break;
      case 30: {
        int z = settings_.zoneHours + dir;
        settings_.zoneHours = int8_t(z < -12 ? -12 : (z > 14 ? 14 : z));
        break;
      }
      case 31:
        settings_.euDst = !settings_.euDst;
        break;
    }
  }

  void valueText(Context& ctx, uint8_t id, char* out, int n) {
    if (id >= 25 && id <= 29 && !ctx.time.settable()) {
      snprintf(out, n, "PC");  // the simulator follows the computer's clock
      return;
    }
    TimeOfDay t = ctx.time.now();
    switch (id) {
      case 0: snprintf(out, n, "%d", settings_.brightness); break;
      case 1:
        snprintf(out, n, "%s", palettes::byIndex(settings_.paletteIndex).name);
        break;
      case 2: snprintf(out, n, "%d", settings_.rotation * 90); break;
      case 3: snprintf(out, n, "%s", settings_.autoplay ? "ON" : "OFF"); break;
      case 4: snprintf(out, n, "%d", settings_.autoplaySeconds); break;
      case 5:
        snprintf(out, n, "%s", settings_.yogaBody == 0 ? "FEM" : "MALE");
        break;
      case 6: snprintf(out, n, "%d", settings_.yogaHoldSec); break;
      case 7:
        snprintf(out, n, "%s", settings_.breatheStyle == 0 ? "BOX" : "4-7-8");
        break;
      case 8: snprintf(out, n, "%d", settings_.breatheSec); break;
      case 9:
        snprintf(out, n, "%s",
                 settings_.pongLevel == 0
                     ? "EASY"
                     : (settings_.pongLevel == 1 ? "NORM" : "HARD"));
        break;
      case 10: snprintf(out, n, "%s", settings_.fireSparks ? "ON" : "OFF"); break;
      case 11: {
        static const char* ep[4] = {"BODY", "KBELL", "TABATA", "CORE"};
        snprintf(out, n, "%s", ep[settings_.exerciseProgram & 3]);
        break;
      }
      case 12: snprintf(out, n, "%d", settings_.exerciseReps); break;
      case 13: snprintf(out, n, "%d", settings_.exerciseRepSec); break;
      case 14:
        snprintf(out, n, "%s",
                 settings_.snakeSpeed == 0
                     ? "SLOW"
                     : (settings_.snakeSpeed == 1 ? "NORM" : "FAST"));
        break;
      case 15: {
        static const char* ot[4] = {"OFF", "DIGI", "ANLG", "WORD"};
        snprintf(out, n, "%s", ot[settings_.overlayType]);
        break;
      }
      case 16: {
        static const char* op[9] = {"TL", "TC", "TR", "ML", "MID",
                                    "MR", "BL", "BC", "BR"};
        snprintf(out, n, "%s", op[settings_.overlayPos]);
        break;
      }
      case 17:
        snprintf(out, n, "%s",
                 settings_.overlaySize == 0
                     ? "SML"
                     : (settings_.overlaySize == 1 ? "MED" : "LRG"));
        break;
      case 18: {
        static const char* fp[5] = {"WAKE", "SUN", "STAND", "HIPS", "BED"};
        snprintf(out, n, "%s", fp[settings_.yogaProgram >= 5 ? 0 : settings_.yogaProgram]);
        break;
      }
      case 19: snprintf(out, n, "%s", settings_.nightMode ? "ON" : "OFF"); break;
      case 20: snprintf(out, n, "%02d:00", settings_.nightStartHour); break;
      case 21: snprintf(out, n, "%02d:00", settings_.nightEndHour); break;
      case 22: snprintf(out, n, "%d", settings_.nightBrightness); break;
      case 23: {
        static const char* tr[4] = {"AUTO", "SLIDE", "FADE", "CUT"};
        snprintf(out, n, "%s", tr[settings_.transitionMode >= 4 ? 0 : settings_.transitionMode]);
        break;
      }
      case 24:
        if (settings_.volume == 0) snprintf(out, n, "MUTE");
        else snprintf(out, n, "%d", settings_.volume);
        break;
      case 25: snprintf(out, n, "%02d", t.hour); break;
      case 26: snprintf(out, n, "%02d", t.minute); break;
      case 27: snprintf(out, n, "%d", t.day); break;
      case 28: snprintf(out, n, "%s", timeutil::monthShort(t.month)); break;
      case 29: snprintf(out, n, "%d", t.year); break;
      case 30:
        snprintf(out, n, "UTC%c%d", settings_.zoneHours < 0 ? '-' : '+',
                 settings_.zoneHours < 0 ? -settings_.zoneHours : settings_.zoneHours);
        break;
      case 31: snprintf(out, n, "%s", settings_.euDst ? "EU AUTO" : "OFF"); break;
      default: out[0] = 0;
    }
  }
};

}

#endif
