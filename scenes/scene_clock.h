// The flagship scene: Coriolis is a wall clock first. Big rounded digits
// with a soft vertical gradient drifting slowly through the palette, a
// colon that breathes instead of blinking, the date beneath, and a slim
// seconds bar. Restrained on purpose — it runs 24/7 in a living room.
#ifndef CORIOLIS_SCENE_CLOCK_H
#define CORIOLIS_SCENE_CLOCK_H

#include <stdio.h>
#include <math.h>

#include "../core/scene.h"
#include "../core/font5x7.h"
#include "../core/math8.h"
#include "../core/timeutil.h"

namespace coriolis {

class ClockScene : public Scene {
 public:
  const char* name() const { return "Clock"; }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();
    const int W = ctx.fb.width(), H = ctx.fb.height();

    TimeOfDay t = ctx.time.now();

    char hh[3], mm[3];
    snprintf(hh, sizeof(hh), "%02d", t.hour % 100);
    snprintf(mm, sizeof(mm), "%02d", t.minute % 100);

    // largest scale that fits with margins, so this works at 64 or 128 tall
    int scale = 1;
    while (font5x7::textWidth("00:00", scale + 1) <= W - 10 &&
           font5x7::GLYPH_H * (scale + 1) <= H - 30) {
      scale++;
    }

    const int digitsW = font5x7::textWidth("00:00", scale);
    const int digitH = font5x7::GLYPH_H * scale;
    int x = (W - digitsW) / 2;
    int y = (H - digitH) / 2 - (H >= 100 ? 8 : 6);

    // hue drifts through the palette over ~4 minutes; boosted so digits
    // stay readable even in a palette's dark regions
    uint8_t hue = uint8_t(ctx.nowMs / 1000);
    RGB top = ctx.palette->lookupBright(hue);
    RGB bottom = ctx.palette->lookupBright(uint8_t(hue + 40));
    bottom.dim(170);

    font5x7::drawText(ctx.fb, hh, x, y, scale, top, bottom);
    x += font5x7::textWidth("00", scale) + scale;

    // the colon breathes once per second rather than snapping on and off
    float k = 0.55f + 0.45f * cosf((ctx.nowMs % 2000) / 2000.0f * TWO_PI_F);
    RGB ct = top, cb = bottom;
    ct.dim(uint8_t(70 + 185 * k));
    cb.dim(uint8_t(70 + 185 * k));
    font5x7::drawChar(ctx.fb, ':', x, y, scale, ct, cb);
    x += font5x7::charAdvance(':') * scale;

    font5x7::drawText(ctx.fb, mm, x, y, scale, top, bottom);

    // the date, quiet beneath the time
    int dateY = y + digitH + (H >= 100 ? 10 : 4);
    char date[16];
    int wd = timeutil::weekdayOf(t.year, t.month, t.day);
    snprintf(date, sizeof(date), "%s %d %s", timeutil::weekdayShort(wd),
             t.day % 100, timeutil::monthShort(t.month));
    RGB dateCol = ctx.palette->lookupBright(uint8_t(hue + 20));
    dateCol.dim(150);
    font5x7::drawCentered(ctx.fb, date, W / 2, dateY, 1, dateCol);

    // seconds: a slim centered bar that fills over the minute
    int barW = W / 2, barX = (W - barW) / 2;
    int barY = dateY + (H >= 100 ? 14 : 9);
    if (barY < H - 1) {
      RGB track = top;
      track.dim(28);
      ctx.fb.hLine(barX, barX + barW - 1, barY, track);
      int fill = (barW * t.second) / 59;
      RGB lit = top;
      lit.dim(150);
      if (fill > 0) ctx.fb.hLine(barX, barX + fill - 1, barY, lit);
    }

    return 50;  // 20 fps keeps the colon's breath smooth
  }
};

}

#endif
