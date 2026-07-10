// Word clock — the time in words, stacked lines, rounded to five minutes:
// "TWENTY FIVE / PAST / SEVEN". The spiritual successor of Borealis's
// ClockText, which ran on the wall for years.
#ifndef CORIOLIS_SCENE_WORDCLOCK_H
#define CORIOLIS_SCENE_WORDCLOCK_H

#include <string.h>

#include "../core/scene.h"
#include "../core/font.h"

namespace coriolis {

class WordClockScene : public Scene {
 public:
  const char* name() const { return "Word Clock"; }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    TimeOfDay t = ctx.time.now();

    // round to the nearest 5 minutes; past the half hour we speak toward
    // the next hour ("twenty to eight")
    int m5 = ((t.minute + 2) / 5) * 5 % 60;
    int hour = t.hour;
    if (t.minute + 2 >= 60) hour++;
    if (m5 > 30) hour++;
    hour %= 12;
    if (hour == 0) hour = 12;

    const char* lines[3];
    int lineCount = 0;

    static const char* hourNames[13] = {
        "",     "ONE", "TWO",   "THREE", "FOUR",   "FIVE",  "SIX",
        "SEVEN", "EIGHT", "NINE", "TEN", "ELEVEN", "TWELVE"};

    if (m5 == 0) {
      lines[0] = hourNames[hour];
      lines[1] = "O'CLOCK";
      lineCount = 2;
    } else if (m5 == 30) {
      lines[0] = "HALF PAST";
      lines[1] = hourNames[hour];
      lineCount = 2;
    } else {
      int spoken = m5 <= 30 ? m5 : 60 - m5;
      const char* minuteName = "";
      switch (spoken) {
        case 5: minuteName = "FIVE"; break;
        case 10: minuteName = "TEN"; break;
        case 15: minuteName = "QUARTER"; break;
        case 20: minuteName = "TWENTY"; break;
        case 25: minuteName = "TWENTY FIVE"; break;
      }
      lines[0] = minuteName;
      lines[1] = m5 < 30 ? "PAST" : "TO";
      lines[2] = hourNames[hour];
      lineCount = 3;
    }

    // largest scale where every line fits
    int scale = 4;
    while (scale > 1) {
      bool fits = true;
      for (int i = 0; i < lineCount; i++) {
        if (font3x5::textWidth(lines[i], scale) > ctx.fb.width() - 6)
          fits = false;
      }
      int totalH = lineCount * (font3x5::GLYPH_H * scale + scale) - scale;
      if (totalH > ctx.fb.height() - 6) fits = false;
      if (fits) break;
      scale--;
    }

    int lineH = font3x5::GLYPH_H * scale + scale;
    int totalH = lineCount * lineH - scale;
    int y = (ctx.fb.height() - totalH) / 2;

    // the hour line pops in full palette color, the rest sit dimmer
    uint8_t hue = uint8_t(ctx.nowMs / 1000);
    for (int i = 0; i < lineCount; i++) {
      bool isHourLine = (i == lineCount - 1) || (lineCount == 2 && i == 0 && m5 == 0);
      RGB color = ctx.palette->lookup(hue, isHourLine ? 255 : 110);
      int x = (ctx.fb.width() - font3x5::textWidth(lines[i], scale)) / 2;
      font3x5::drawText(ctx.fb, lines[i], x, y, scale, color);
      y += lineH;
    }

    return 250;
  }
};

}

#endif
