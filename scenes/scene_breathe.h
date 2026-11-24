// Breathing guide — a luminous orb to breathe with. Two styles, selectable
// in settings: box breathing (equal in/hold/out/hold) and the 4-7-8 relaxing
// breath (long hold, longer exhale, brief bottom rest). Up/down adjusts the
// in-breath length; everything else scales from it.
#ifndef CORIOLIS_SCENE_BREATHE_H
#define CORIOLIS_SCENE_BREATHE_H

#include <stdio.h>
#include <math.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/font5x7.h"
#include "../core/math8.h"
#include "../core/settings.h"
#include "guide_ui.h"
#include "intro.h"

namespace coriolis {

class BreatheScene : public Scene {
 public:
  explicit BreatheScene(Settings& settings) : settings_(settings) {}

  const char* name() const { return "Breathe"; }
  bool autoplayEligible() const { return false; }

  void start(Context&) { started_ = false; }

  bool input(Context& ctx, Key k) {
    if (!started_) {
      if (k == Key::Select) {
        started_ = true;
        cycleStartMs_ = ctx.nowMs;
        cycles_ = 0;
        lastPhase_ = -1;
        ctx.audio.play(Cue::StartBell);
        return true;
      }
      if (k == Key::Up || k == Key::Down) {
        int s = settings_.breatheSec + ((k == Key::Up) ? 1 : -1);
        settings_.breatheSec = uint8_t(s < 3 ? 3 : (s > 8 ? 8 : s));
        return true;
      }
      return false;
    }
    if (k == Key::Back) { started_ = false; return true; }
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
    if (!started_) {
      char l0[16], l1[16];
      snprintf(l0, sizeof(l0), "%s",
               settings_.breatheStyle == 0 ? "BOX BREATH" : "4-7-8 BREATH");
      snprintf(l1, sizeof(l1), "%d SEC BASE", settings_.breatheSec);
      const char* lines[] = {l0, l1};
      intro::draw(ctx, "BREATHE", lines, 2, guide::titleColor());
      return 60;
    }

    ctx.fb.clear();
    const int W = ctx.fb.width(), H = ctx.fb.height();

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
    float phaseProg = (inCycle - acc) / float(phaseLen[phase]);
    float t = phaseProg * phaseProg * (3.0f - 2.0f * phaseProg);  // smoothstep

    // a soft cue as each phase begins
    if (phase != lastPhase_) {
      lastPhase_ = phase;
      static const Cue cues[4] = {Cue::BreatheIn, Cue::BreatheHold,
                                  Cue::BreatheOut, Cue::BreatheHold};
      ctx.audio.play(cues[phase]);
    }

    static const RGB phaseColors[4] = {
        RGB(64, 215, 200),   // inhale: serene teal
        RGB(185, 155, 255),  // hold: soft lavender
        RGB(115, 130, 235),  // exhale: twilight indigo
        RGB(95, 145, 155),   // rest: gentle slate-teal
    };
    RGB phaseCol = phaseColors[phase];

    int size = W < H ? W : H;
    int minR = size / 9;
    int maxR = size / 2 - (H >= 100 ? 18 : 11);

    float f;                        // 0 = small, 1 = full
    switch (phase) {
      case 0: f = t; break;
      case 1: f = 1.0f; break;
      case 2: f = 1.0f - t; break;
      default: f = 0.0f; break;
    }
    float r = minR + (maxR - minR) * f;

    int cx = W / 2;
    int cy = H / 2 + (H >= 100 ? 1 : 2);

    // layered radial aura
    RGB outer = phaseCol; outer.dim(26);
    RGB mid = phaseCol;   mid.dim(55);
    RGB inner = phaseCol; inner.dim(95);
    RGB core = lerpColor(phaseCol, RGB(255, 255, 255), 85);
    core.dim(125);

    // discs take continuous coordinates, arcs the pixel itself: +0.5 lines
    // the two up on the same center
    const float ox = cx + 0.5f, oy = cy + 0.5f;
    ctx.fb.fillCircleAA(ox, oy, r * 1.18f, outer);
    ctx.fb.fillCircleAA(ox, oy, r * 1.02f, mid);
    ctx.fb.fillCircleAA(ox, oy, r * 0.84f, inner);
    ctx.fb.fillCircleAA(ox, oy, r * 0.45f, core);

    // crisp outer rim on the breathing sphere and an outer phase-progress arc
    ctx.fb.drawArc(cx, cy, int(r + 0.5f), 1, 0.0f, TWO_PI_F, phaseCol);

    RGB track = phaseCol;
    track.dim(35);
    int arcR = maxR + (H >= 100 ? 4 : 2);
    ctx.fb.drawArc(cx, cy, arcR, 2, 0.0f, TWO_PI_F, track);
    ctx.fb.drawArc(cx, cy, arcR, 2, -1.5708f, TWO_PI_F * phaseProg, phaseCol);

    // calm anchor dot in the middle
    ctx.fb.fillCircleAA(ox, oy, 2.2f, RGB(230, 245, 245));

    guide::drawTopBar(ctx, settings_.breatheStyle == 0 ? "BREATHE" : "4-7-8");

    static const char* phaseNames[4] = {"Inhale", "Hold", "Exhale", "Rest"};
    const char* label = phaseNames[phase];
    if (H >= 100) {
      font5x7::drawCentered(ctx.fb, label, W / 2, H - 12, 1, phaseCol);
    } else {
      static const char* shortNames[4] = {"IN", "HOLD", "OUT", "REST"};
      int w = font3x5::textWidth(shortNames[phase], 1);
      font3x5::drawText(ctx.fb, shortNames[phase], (W - w) / 2, H - 7, 1,
                        phaseCol);
    }

    // phase seconds remaining bottom-left, completed-cycle count bottom-right
    int secLeft = int((phaseLen[phase] - (inCycle - acc) + 999) / 1000);
    char leftBuf[16];
    snprintf(leftBuf, sizeof(leftBuf), "%ds", secLeft);
    font3x5::drawText(ctx.fb, leftBuf, 3, H - 7, 1, guide::mutedColor());

    char count[8];
    snprintf(count, sizeof(count), "%d", cycles_);
    int cw = font3x5::textWidth(count, 1);
    font3x5::drawText(ctx.fb, count, W - cw - 3, H - 7, 1, guide::mutedColor());

    return 33;
  }

 private:
  Settings& settings_;
  bool started_ = false;
  int lastPhase_ = -1;
  uint32_t cycleStartMs_ = 0;
  int cycles_ = 0;
};

}

#endif
