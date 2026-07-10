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

    // dial: twelve ticks, the quarters heavier
    RGB tickColor = ctx.palette->lookupBright(hue, 140);
    for (int i = 0; i < 12; i++) {
      float a = i * (TWO_PI_F / 12.0f);
      float sa = sinf(a), ca = cosf(a);
      int inner = (i % 3 == 0) ? r - 5 : r - 2;
      ctx.fb.line(cx + int(sa * inner), cy - int(ca * inner),
                  cx + int(sa * r), cy - int(ca * r), tickColor);
    }

    // hands: hour short and heavy, minute long and heavy, second thin
    float ah = ((t.hour % 12) + t.minute / 60.0f) * (TWO_PI_F / 12.0f);
    float am = (t.minute + t.second / 60.0f) * (TWO_PI_F / 60.0f);
    float as = t.second * (TWO_PI_F / 60.0f);

    RGB handColor = ctx.palette->lookupBright(hue);
    ctx.fb.thickLine(cx, cy, cx + int(sinf(ah) * r * 0.52f),
                     cy - int(cosf(ah) * r * 0.52f), handColor);
    ctx.fb.thickLine(cx, cy, cx + int(sinf(am) * r * 0.80f),
                     cy - int(cosf(am) * r * 0.80f), handColor);

    RGB secondColor = ctx.palette->lookupBright(uint8_t(hue + 128));
    ctx.fb.line(cx, cy, cx + int(sinf(as) * r * 0.88f),
                cy - int(cosf(as) * r * 0.88f), secondColor);

    ctx.fb.fillCircle(cx, cy, 2, handColor);

    return 100;
  }
};

}

#endif
