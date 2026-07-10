// The flagship scene: Coriolis is a wall clock first. Big chunky digits,
// blinking colon, a thin seconds bar, digit color drifting slowly through
// the palette. Restrained on purpose — it runs 24/7 in a living room.
#ifndef CORIOLIS_SCENE_CLOCK_H
#define CORIOLIS_SCENE_CLOCK_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"

namespace coriolis {

class ClockScene : public Scene {
 public:
  const char* name() const { return "Clock"; }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    TimeOfDay t = ctx.time.now();

    char text[6];
    snprintf(text, sizeof(text), "%02d:%02d", t.hour, t.minute);

    // largest scale that fits with margins, so this works at 64 or 128 wide
    int scale = 1;
    while (font3x5::textWidth(text, scale + 1) <= ctx.fb.width() - 8 &&
           (font3x5::GLYPH_H * (scale + 1)) <= ctx.fb.height() - 12) {
      scale++;
    }

    int w = font3x5::textWidth(text, scale);
    int x = (ctx.fb.width() - w) / 2;
    int y = (ctx.fb.height() - font3x5::GLYPH_H * scale) / 2;

    // hue drifts through the palette over ~4 minutes
    uint8_t hue = uint8_t(ctx.nowMs / 1000);
    RGB digitColor = ctx.palette->lookup(hue);

    // colon blinks on even seconds: draw time in two parts around it
    bool colonOn = (t.second % 2) == 0;
    for (int i = 0; text[i]; i++) {
      char c = text[i];
      if (c == ':' && !colonOn) c = ' ';
      x += font3x5::drawChar(ctx.fb, c, x, y, scale, digitColor);
    }

    // seconds progress bar along the bottom edge, dimmed
    int barWidth = (ctx.fb.width() * t.second) / 59;
    RGB barColor = digitColor;
    barColor.dim(64);
    ctx.fb.hLine(0, barWidth, ctx.fb.height() - 1, barColor);

    return 100;  // 10 fps is plenty for a clock
  }
};

}

#endif
