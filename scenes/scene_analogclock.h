// Analog clock face — hour, minute and second hands on a ticked dial.
// The successor of Borealis's PatternAnalogClock2, which earned one of the
// two pattern slots that fit on the old hardware.
#ifndef CORIOLIS_SCENE_ANALOGCLOCK_H
#define CORIOLIS_SCENE_ANALOGCLOCK_H

#include <math.h>

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class AnalogClockScene : public Scene {
 public:
  const char* name() const { return "Analog"; }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    const int cx = ctx.fb.width() / 2;
    const int cy = ctx.fb.height() / 2;
    int r = (ctx.fb.width() < ctx.fb.height() ? ctx.fb.width()
                                              : ctx.fb.height()) / 2 - 3;

    TimeOfDay t = ctx.time.now();
    uint8_t hue = uint8_t(ctx.nowMs / 1000);

    // dial: subtle 60-minute dots plus twelve hour ticks, quarters heavier
    RGB tickColor = ctx.palette->lookupBright(hue, 150);
    RGB minColor = tickColor;
    minColor.dim(65);
    for (int i = 0; i < 60; i++) {
      float a = i * (TWO_PI_F / 60.0f);
      float sa = sinf(a), ca = cosf(a);
      if (i % 5 == 0) {
        int h = i / 5;
        int inner = (h % 3 == 0) ? r - 5 : r - 3;
        if (h % 3 == 0) {
          ctx.fb.thickLine(cx + int(sa * inner), cy - int(ca * inner),
                           cx + int(sa * r), cy - int(ca * r), tickColor);
        } else {
          ctx.fb.line(cx + int(sa * inner), cy - int(ca * inner),
                      cx + int(sa * r), cy - int(ca * r), tickColor);
        }
      } else if (r >= 24) {
        ctx.fb.set(cx + int(sa * r), cy - int(ca * r), minColor);
      }
    }

    // hands: hour short and heavy, minute long, second slim with counterweight
    float ah = ((t.hour % 12) + t.minute / 60.0f) * (TWO_PI_F / 12.0f);
    float am = (t.minute + t.second / 60.0f) * (TWO_PI_F / 60.0f);
    float as = t.second * (TWO_PI_F / 60.0f);

    RGB handColor = ctx.palette->lookupBright(hue);
    RGB hourColor = lerpColor(handColor, RGB(255, 255, 255), 40);
    int hx = cx + int(sinf(ah) * r * 0.52f);
    int hy = cy - int(cosf(ah) * r * 0.52f);
    ctx.fb.boldLine(cx, cy, hx, hy, hourColor);
    ctx.fb.fillCircleAA(hx + 0.5f, hy + 0.5f, 1.4f, hourColor);

    int mx = cx + int(sinf(am) * r * 0.78f);
    int my = cy - int(cosf(am) * r * 0.78f);
    ctx.fb.thickLine(cx, cy, mx, my, handColor);
    ctx.fb.fillCircleAA(mx + 0.5f, my + 0.5f, 1.1f, handColor);

    RGB secondColor = ctx.palette->lookupBright(uint8_t(hue + 128));
    ctx.fb.line(cx - int(sinf(as) * r * 0.16f),
                cy + int(cosf(as) * r * 0.16f),
                cx + int(sinf(as) * r * 0.88f),
                cy - int(cosf(as) * r * 0.88f), secondColor);

    ctx.fb.fillCircleAA(cx + 0.5f, cy + 0.5f, 2.5f, handColor);
    ctx.fb.fillCircleAA(cx + 0.5f, cy + 0.5f, 1.1f, secondColor);

    return 100;
  }
};

}

#endif
