// Bounce — the eternal DVD-logo screensaver, but the thing ricocheting
// around is the clock. It drifts, bounces off the walls changing color on
// each hit, and the display flashes when it finally nails a perfect corner.
// A screensaver that also happens to tell the time.
#ifndef CORIOLIS_SCENE_BOUNCE_H
#define CORIOLIS_SCENE_BOUNCE_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"

namespace coriolis {

class BounceScene : public Scene {
 public:
  const char* name() const { return "Bounce"; }

  void start(Context& ctx) {
    x_ = 6;
    y_ = 6;
    vx_ = 1;
    vy_ = 1;
    hue_ = random8();
    cornerFlash_ = 0;
  }

  uint32_t draw(Context& ctx) {
    const int W = ctx.fb.width(), H = ctx.fb.height();
    const int lw = font3x5::textWidth("00:00", SCALE);
    const int lh = font3x5::GLYPH_H * SCALE;

    x_ += vx_;
    y_ += vy_;

    bool hitX = false, hitY = false;
    if (x_ <= 0) { x_ = 0; vx_ = 1; hitX = true; }
    else if (x_ + lw >= W) { x_ = W - lw; vx_ = -1; hitX = true; }
    if (y_ <= 0) { y_ = 0; vy_ = 1; hitY = true; }
    else if (y_ + lh >= H) { y_ = H - lh; vy_ = -1; hitY = true; }

    if (hitX || hitY) hue_ += 40 + random8(40);  // new color on every bounce
    if (hitX && hitY) {                          // a corner! the holy grail
      cornerFlash_ = 12;
      ctx.audio.play(Cue::Score);
    }

    ctx.fb.clear();
    if (cornerFlash_ > 0) {
      RGB f = ctx.palette->lookupBright(hue_);
      f.dim(uint8_t(cornerFlash_ * 18));
      ctx.fb.fill(f);  // full-screen flash, fading out
      cornerFlash_--;
    }

    TimeOfDay t = ctx.time.now();
    char txt[6];
    snprintf(txt, sizeof(txt), "%02d:%02d", t.hour, t.minute);
    font3x5::drawText(ctx.fb, txt, x_, y_, SCALE, ctx.palette->lookupBright(hue_));

    return 40;
  }

 private:
  static const int SCALE = 3;  // chunky bouncing digits
  int x_, y_, vx_, vy_, cornerFlash_;
  uint8_t hue_;
};

}

#endif
