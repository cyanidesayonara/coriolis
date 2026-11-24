// Focus — a Pomodoro timer for the wall. Work and break phases count down in
// big rounded digits, color-coded (warm for work, green for a break), with an
// anti-aliased ring that drains as the phase runs and a row of tomatoes for
// completed sessions. Glanceable from across the room.
#ifndef CORIOLIS_SCENE_FOCUS_H
#define CORIOLIS_SCENE_FOCUS_H

#include <stdio.h>
#include <math.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/font5x7.h"
#include "../core/math8.h"
#include "intro.h"

namespace coriolis {

class FocusScene : public Scene {
 public:
  const char* name() const { return "Focus"; }
  bool autoplayEligible() const { return false; }

  void start(Context&) { started_ = false; }

  void begin(Context& ctx) {
    started_ = true;
    paused_ = false;
    phase_ = 0;  // work
    pomodoros_ = 0;
    phaseStartMs_ = ctx.nowMs;
    ctx.audio.play(Cue::StartBell);
  }

  bool input(Context& ctx, Key k) {
    if (!started_) {
      if (k == Key::Select) { begin(ctx); return true; }
      if (k == Key::Up || k == Key::Down) {
        workMin_ += (k == Key::Up) ? 5 : -5;
        if (workMin_ < 5) workMin_ = 5;
        if (workMin_ > 60) workMin_ = 60;
        return true;
      }
      return false;
    }
    if (k == Key::Select) { paused_ = !paused_; return true; }
    if (k == Key::Back) { started_ = false; return true; }
    return false;
  }

  uint32_t draw(Context& ctx) {
    if (!started_) {
      char l0[16];
      snprintf(l0, sizeof(l0), "WORK %d MIN", workMin_);
      const char* lines[] = {l0, "BREAK 5  LONG 15"};
      intro::draw(ctx, "FOCUS", lines, 2, RGB(230, 120, 90));
      return 60;
    }

    uint32_t phaseMs = phaseLenMin() * 60000u;
    uint32_t elapsed = ctx.nowMs - phaseStartMs_;
    if (paused_) phaseStartMs_ = ctx.nowMs - elapsed;

    if (!paused_ && elapsed >= phaseMs) {
      advance(ctx);
      elapsed = 0;
      phaseMs = phaseLenMin() * 60000u;
    }

    uint32_t remain = phaseMs - elapsed;
    int mins = int(remain / 60000u);
    int secs = int((remain / 1000u) % 60u);

    ctx.fb.clear();
    const int W = ctx.fb.width(), H = ctx.fb.height();
    RGB accent = phaseColor();

    // phase label clear above the ring
    const char* label = phase_ == 0 ? "Focus" : (phase_ == 1 ? "Break" : "Long break");
    if (H >= 100) {
      font5x7::drawCentered(ctx.fb, label, W / 2, 3, 1, accent);
    } else {
      const char* sLabel = phase_ == 0 ? "FOCUS" : (phase_ == 1 ? "BREAK" : "LONG");
      int lw = font3x5::textWidth(sLabel, 1);
      font3x5::drawText(ctx.fb, sLabel, (W - lw) / 2, 2, 1, accent);
    }

    // smooth anti-aliased draining ring around the timer
    int minDim = W < H ? W : H;
    int cx = W / 2;
    int cy = H / 2 + (H >= 100 ? 4 : 2);
    int r = minDim / 2 - (H >= 100 ? 17 : 10);
    float frac = 1.0f - float(elapsed) / float(phaseMs);
    RGB ringDim = accent;
    ringDim.dim(45);
    ctx.fb.drawArc(cx, cy, r, H >= 100 ? 3 : 2, 0.0f, TWO_PI_F, ringDim);
    ctx.fb.drawArc(cx, cy, r, H >= 100 ? 3 : 2, -1.5708f, TWO_PI_F * frac,
                   accent);

    // MM:SS in rounded numerals inside the ring
    char t[16];
    snprintf(t, sizeof(t), "%d:%02d", mins, secs);
    int scale = H >= 100 ? 3 : 2;
    while (scale > 1 && font5x7::textWidth(t, scale) > (r - 5) * 2) scale--;
    RGB digits =
        paused_ && (ctx.nowMs / 400) % 2 ? RGB(85, 85, 92) : RGB(245, 245, 248);
    RGB digitsBot = lerpColor(digits, accent, 80);
    int ty = cy - font5x7::GLYPH_H * scale / 2;
    font5x7::drawCentered(ctx.fb, t, cx, ty, scale, digits, digitsBot);

    if (paused_ && H >= 100) {
      const char* p = "PAUSED";
      int pw = font3x5::textWidth(p, 1);
      font3x5::drawText(ctx.fb, p, cx - pw / 2,
                        ty + font5x7::GLYPH_H * scale + 5, 1, accent);
    }

    // completed pomodoros as a row of tomato dots at the bottom
    for (int i = 0; i < pomodoros_ && i < 8; i++) {
      int tx = W / 2 - (pomodoros_ < 8 ? pomodoros_ : 8) * 3 + i * 6 + 3;
      ctx.fb.fillCircleAA(float(tx), float(H - 6), 2.0f, RGB(230, 75, 65));
    }

    return 100;
  }

 private:
  bool started_ = false, paused_ = false;
  int phase_ = 0;  // 0 work, 1 short break, 2 long break
  int pomodoros_ = 0;
  int workMin_ = 25;
  uint32_t phaseStartMs_ = 0;

  int phaseLenMin() const { return phase_ == 0 ? workMin_ : (phase_ == 1 ? 5 : 15); }

  RGB phaseColor() const {
    return phase_ == 0 ? RGB(240, 120, 80)
                       : (phase_ == 1 ? RGB(70, 200, 110) : RGB(80, 170, 230));
  }

  void advance(Context& ctx) {
    if (phase_ == 0) {
      pomodoros_++;
      phase_ = (pomodoros_ % 4 == 0) ? 2 : 1;  // long break every 4th
      ctx.audio.play(Cue::FinishBell);
    } else {
      phase_ = 0;
      ctx.audio.play(Cue::StartBell);
    }
    phaseStartMs_ = ctx.nowMs;
  }
};

}

#endif
