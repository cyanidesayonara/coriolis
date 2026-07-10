// Shared look for the guided-activity scenes (yoga, exercise, breathing).
// These scenes keep a fixed, friendly identity — coral figure, teal mat,
// lavender titles — independent of the theme palette. Only the top bar's
// clock is themed, so the display still feels consistent with the rest.
#ifndef CORIOLIS_GUIDE_UI_H
#define CORIOLIS_GUIDE_UI_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"

namespace coriolis {
namespace guide {

inline RGB bodyColor() { return RGB(255, 130, 100); }   // warm coral
inline RGB limbColor() { return RGB(235, 90, 120); }    // deeper rose
inline RGB matColor() { return RGB(0, 185, 165); }      // teal
inline RGB titleColor() { return RGB(195, 160, 255); }  // soft lavender
inline RGB mutedColor() { return RGB(110, 110, 110); }

// top bar: themed clock left, fixed-identity title centered.
// Returns the y where scene content may start.
inline int drawTopBar(Context& ctx, const char* title) {
  TimeOfDay t = ctx.time.now();
  char clock[6];
  snprintf(clock, sizeof(clock), "%02d:%02d", t.hour, t.minute);
  font3x5::drawText(ctx.fb, clock, 2, 2, 1, ctx.palette->lookupBright(0));

  int w = font3x5::textWidth(title, 1);
  font3x5::drawText(ctx.fb, title, (ctx.fb.width() - w) / 2, 2, 1,
                    titleColor());
  return 9;
}

}
}

#endif
