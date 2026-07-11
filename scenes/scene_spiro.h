// Spirograph trails — a port of Borealis's PatternSpiro (the pattern that
// actually ran on the wall for years), reworked against the Coriolis API.
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
    theta1_ = theta2_ = hue_ = 0;
    count_ = 1;
    growing_ = true;
    lastStepMs_ = lastCountMs_ = 0;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.dimAll(250);

    const int rx = ctx.fb.width() / 4;
    const int ry = ctx.fb.height() / 4;
    uint8_t offset = uint8_t(256 / count_);

    for (int i = 0; i < count_; i++) {
      uint8_t phase = uint8_t(theta1_ + i * offset);
      int cx = CENTER_X - rx + (sin8(phase) * 2 * rx) / 255;
      int cy = CENTER_Y - ry + (cos8(phase) * 2 * ry) / 255;

      uint8_t phase2 = uint8_t(theta2_ + i * offset);
      int x = cx - rx / 2 + (sin8(phase2) * rx) / 255;
      int y = cy - ry / 2 + (cos8(phase2) * ry) / 255;

      RGB c = ctx.palette->lookup(uint8_t(hue_ + i * offset), 128);
      ctx.fb.at(x, y).add(c);
    }

    theta2_ += 2;

    if (ctx.nowMs - lastStepMs_ >= 12) {
      lastStepMs_ = ctx.nowMs;
      theta1_ += 1;
      hue_ += 1;
    }

    // the strand count pulses up and back down on a loop — the signature
    // Borealis behavior (the original tied this to sweeps through the
    // center, which almost never land exactly on a pixel at 128x128)
    if (ctx.nowMs - lastCountMs_ >= 2200) {
      lastCountMs_ = ctx.nowMs;
      if (count_ >= 32) growing_ = false;
      else if (count_ <= 1) growing_ = true;
      count_ = growing_ ? count_ * 2 : count_ / 2;
      if (count_ < 1) count_ = 1;
    }

    return 15;
  }

 private:
  uint8_t theta1_, theta2_, hue_;
  int count_;
  bool growing_;
  uint32_t lastStepMs_, lastCountMs_;
};

}

#endif
