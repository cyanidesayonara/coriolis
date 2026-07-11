// Breathing guide — a circle to breathe with. Two styles, selectable in
// settings: box breathing (equal in/hold/out/hold) and the 4-7-8 relaxing
// breath (long hold, longer exhale, no bottom hold). Up/down adjusts the
// in-breath length; everything else scales from it.
#ifndef CORIOLIS_SCENE_BREATHE_H
#define CORIOLIS_SCENE_BREATHE_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/settings.h"
#include "guide_ui.h"

namespace coriolis {

class BreatheScene : public Scene {
 public:
  explicit BreatheScene(Settings& settings) : settings_(settings) {}

  const char* name() const { return "Breathe"; }
  bool autoplayEligible() const { return false; }

  void start(Context& ctx) {
    cycleStartMs_ = ctx.nowMs;
    cycles_ = 0;
  }

  bool input(Context&, Key k) {
    if (k == Key::Up || k == Key::Down) {  // in-breath length, 3-8s
      int s = settings_.breatheSec + ((k == Key::Up) ? 1 : -1);
      if (s < 3) s = 3;
      if (s > 8) s = 8;
      settings_.breatheSec = uint8_t(s);
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    // phase lengths in ms; 4-7-8 scales its classic ratios from the
    // in-breath so one knob controls the pace of both styles
    uint32_t unit = uint32_t(settings_.breatheSec) * 1000 / 4;
    uint32_t phaseLen[4];
    if (settings_.breatheStyle == 0) {
      phaseLen[0] = phaseLen[1] = phaseLen[2] = phaseLen[3] = unit * 4;
    } else {
      phaseLen[0] = unit * 4;   // in 4
      phaseLen[1] = unit * 7;   // hold 7
      phaseLen[2] = unit * 8;   // out 8
      phaseLen[3] = unit * 1;   // brief bottom rest
    }
    uint32_t total = phaseLen[0] + phaseLen[1] + phaseLen[2] + phaseLen[3];

    uint32_t sinceStart = ctx.nowMs - cycleStartMs_;
    cycles_ = int(sinceStart / total);
    uint32_t inCycle = sinceStart % total;

    int phase = 0;
    uint32_t acc = 0;
    for (int i = 0; i < 4; i++) {
      if (inCycle < acc + phaseLen[i]) { phase = i; break; }
      acc += phaseLen[i];
    }
    float t = (inCycle - acc) / float(phaseLen[phase]);
    t = t * t * (3.0f - 2.0f * t);  // ease the growth

    int size = ctx.fb.width() < ctx.fb.height() ? ctx.fb.width()
                                                : ctx.fb.height();
    int minR = size / 10;
    int maxR = size / 2 - 14;

    float f;                        // 0 = small, 1 = full
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

    guide::drawTopBar(ctx, settings_.breatheStyle == 0 ? "BREATHE" : "4-7-8");

    static const char* phaseNames[4] = {"IN", "HOLD", "OUT", "HOLD"};
    const char* label = phaseNames[phase];
    int w = font3x5::textWidth(label, 2);
    font3x5::drawText(ctx.fb, label, (ctx.fb.width() - w) / 2,
                      ctx.fb.height() - 14, 2, guide::mutedColor());

    // completed-cycle counter, bottom right
    char count[8];
    snprintf(count, sizeof(count), "%d", cycles_);
    int cw = font3x5::textWidth(count, 1);
    font3x5::drawText(ctx.fb, count, ctx.fb.width() - cw - 3,
                      ctx.fb.height() - 8, 1, guide::mutedColor());

    return 33;
  }

 private:
  Settings& settings_;
  uint32_t cycleStartMs_ = 0;
  int cycles_ = 0;
};

}

#endif
