// Word clock — the exact time spelled out in stacked words, Borealis
// ClockText style: hour first, then the minutes ("FOUR / TWENTY / THREE",
// "FOUR / OH / FIVE", "FOUR / O'CLOCK"). No rounding.
#ifndef CORIOLIS_SCENE_WORDCLOCK_H
#define CORIOLIS_SCENE_WORDCLOCK_H

#include "../core/scene.h"
#include "../core/font.h"

namespace coriolis {

class WordClockScene : public Scene {
 public:
  const char* name() const { return "Word Clock"; }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    TimeOfDay t = ctx.time.now();

    static const char* small[20] = {
        "ZERO",    "ONE",     "TWO",       "THREE",    "FOUR",
        "FIVE",    "SIX",     "SEVEN",     "EIGHT",    "NINE",
        "TEN",     "ELEVEN",  "TWELVE",    "THIRTEEN", "FOURTEEN",
        "FIFTEEN", "SIXTEEN", "SEVENTEEN", "EIGHTEEN", "NINETEEN"};
    static const char* tens[4] = {"TWENTY", "THIRTY", "FORTY", "FIFTY"};

    int hour = t.hour % 12;
    if (hour == 0) hour = 12;

    const char* lines[3];
    int lineCount = 0;

    lines[lineCount++] = small[hour];

    int m = t.minute;
    if (m == 0) {
      lines[lineCount++] = "O'CLOCK";
    } else if (m < 10) {
      lines[lineCount++] = "OH";
      lines[lineCount++] = small[m];
    } else if (m < 20) {
      lines[lineCount++] = small[m];
    } else {
      lines[lineCount++] = tens[m / 10 - 2];
      if (m % 10 != 0) lines[lineCount++] = small[m % 10];
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

    // the hour line pops at full brightness, the minute words sit dimmer
    uint8_t hue = uint8_t(ctx.nowMs / 1000);
    for (int i = 0; i < lineCount; i++) {
      RGB color = ctx.palette->lookupBright(hue);
      if (i > 0) color.dim(150);
      int x = (ctx.fb.width() - font3x5::textWidth(lines[i], scale)) / 2;
      font3x5::drawText(ctx.fb, lines[i], x, y, scale, color);
      y += lineH;
    }

    return 250;
  }
};

}

#endif
