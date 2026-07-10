// Breathing guide — a circle to breathe with. Box breathing: inhale while
// it grows, hold, exhale while it shrinks, hold. Zero text needed beyond
// the phase word; ideally suited to a light on a wall. Second member of
// the guided-activities family, wearing the same fixed look.
#ifndef CORIOLIS_SCENE_BREATHE_H
#define CORIOLIS_SCENE_BREATHE_H

#include "../core/scene.h"
#include "../core/font.h"
#include "guide_ui.h"

namespace coriolis {

class BreatheScene : public Scene {
 public:
  const char* name() const { return "Breathe"; }
  bool autoplayEligible() const { return false; }

  void start(Context& ctx) { cycleStartMs_ = ctx.nowMs; }

  bool input(Context&, Key k) {
    if (k == Key::Up || k == Key::Down) {  // phase length, 3-8s
      phaseSec_ += (k == Key::Up) ? 1 : -1;
      if (phaseSec_ < 3) phaseSec_ = 3;
      if (phaseSec_ > 8) phaseSec_ = 8;
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    const uint32_t phaseMs = uint32_t(phaseSec_) * 1000;
    uint32_t inCycle = (ctx.nowMs - cycleStartMs_) % (phaseMs * 4);
    int phase = int(inCycle / phaseMs);          // 0 in, 1 hold, 2 out, 3 hold
    float t = (inCycle % phaseMs) / float(phaseMs);
    t = t * t * (3.0f - 2.0f * t);               // ease the growth

    int size = ctx.fb.width() < ctx.fb.height() ? ctx.fb.width()
                                                : ctx.fb.height();
    int minR = size / 10;
    int maxR = size / 2 - 14;

    float f;                                     // 0 = small, 1 = full
    switch (phase) {
      case 0: f = t; break;
      case 1: f = 1.0f; break;
      case 2: f = 1.0f - t; break;
      default: f = 0.0f; break;
    }
    int r = minR + int((maxR - minR) * f);

    int cx = ctx.fb.width() / 2;
    int cy = ctx.fb.height() / 2 + 4;

    // soft lavender fill with a bright rim; a still center dot to focus on
    RGB fill = guide::titleColor();
    fill.dim(70);
    ctx.fb.fillCircle(cx, cy, r, fill);
    ctx.fb.circle(cx, cy, r, guide::titleColor());
    ctx.fb.fillCircle(cx, cy, 2, guide::matColor());

    guide::drawTopBar(ctx, "BREATHE");

    static const char* phaseNames[4] = {"IN", "HOLD", "OUT", "HOLD"};
    const char* label = phaseNames[phase];
    int w = font3x5::textWidth(label, 2);
    font3x5::drawText(ctx.fb, label, (ctx.fb.width() - w) / 2,
                      ctx.fb.height() - 14, 2, guide::mutedColor());

    return 33;
  }

 private:
  uint32_t cycleStartMs_ = 0;
  int phaseSec_ = 4;  // classic box breathing
};

}

#endif
