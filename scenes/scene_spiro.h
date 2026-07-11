// Spirograph trails — a faithful port of Borealis's PatternSpiro (which ran
// on the wall for years). Two nested oscillators trace looping strands; the
// strand count winds up and back down as arms sweep through the center.
#ifndef CORIOLIS_SCENE_SPIRO_H
#define CORIOLIS_SCENE_SPIRO_H

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class SpiroScene : public Scene {
 public:
  const char* name() const { return "Spiro"; }

  void start(Context& ctx) {
    ctx.fb.clear();
    theta1_ = theta2_ = hueoffset_ = 0;
    spirocount_ = 1;
    spiroincrement_ = false;
    handledChange_ = false;
    lastTheta1_ = lastHue_ = lastCount_ = ctx.nowMs;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.dimAll(250);

    const int W = ctx.fb.width(), H = ctx.fb.height();
    const int radiusx = W / 4, radiusy = H / 4;
    const int minx = W / 2 - radiusx, maxx = W / 2 + radiusx + 1;
    const int miny = H / 2 - radiusy, maxy = H / 2 + radiusy + 1;
    const int cx = W / 2, cy = H / 2;

    uint8_t spirooffset = uint8_t(256 / spirocount_);

    bool change = false;
    for (int i = 0; i < spirocount_; i++) {
      uint8_t a = uint8_t(theta1_ + i * spirooffset);
      int x = rescale8(sin8(a), uint8_t(minx), uint8_t(maxx));
      int y = rescale8(cos8(a), uint8_t(miny), uint8_t(maxy));

      uint8_t b = uint8_t(theta2_ + i * spirooffset);
      int x2 = rescale8(sin8(b), uint8_t(x - radiusx), uint8_t(x + radiusx));
      int y2 = rescale8(cos8(b), uint8_t(y - radiusy), uint8_t(y + radiusy));

      RGB c = ctx.palette->lookup(uint8_t(hueoffset_ + i * spirooffset), 128);
      ctx.fb.at(x2, y2).add(c);

      if ((x2 == cx && y2 == cy) || (x2 == cx - 1 && y2 == cy - 1))
        change = true;
    }

    theta2_ += 2;

    if (ctx.nowMs - lastTheta1_ >= 12) {
      lastTheta1_ = ctx.nowMs;
      theta1_ += 1;
    }

    if (ctx.nowMs - lastCount_ >= 75) {
      lastCount_ = ctx.nowMs;
      if (change && !handledChange_) {
        handledChange_ = true;
        if (spirocount_ >= W || spirocount_ == 1)
          spiroincrement_ = !spiroincrement_;
        if (spiroincrement_)
          spirocount_ = spirocount_ >= 4 ? spirocount_ * 2 : spirocount_ + 1;
        else
          spirocount_ = spirocount_ > 4 ? spirocount_ / 2 : spirocount_ - 1;
      }
      if (!change) handledChange_ = false;
    }

    if (ctx.nowMs - lastHue_ >= 33) {
      lastHue_ = ctx.nowMs;
      hueoffset_ += 1;
    }

    return 0;
  }

 private:
  uint8_t theta1_, theta2_, hueoffset_;
  int spirocount_;
  bool spiroincrement_, handledChange_;
  uint32_t lastTheta1_, lastHue_, lastCount_;
};

}

#endif
