// Focus — a Pomodoro timer for the wall. Work and break phases count down in
// big digits, color-coded (warm for work, green for a break), with a ring
// that drains as the phase runs and a row of tomatoes for completed sessions.
// Glanceable from across the room, which is the whole point of a wall timer.
#ifndef CORIOLIS_SCENE_FOCUS_H
#define CORIOLIS_SCENE_FOCUS_H

#include <stdio.h>
#include <math.h>

#include "../core/scene.h"
#include "../core/font.h"
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

    // phase label
    const char* label = phase_ == 0 ? "FOCUS" : (phase_ == 1 ? "BREAK" : "LONG BREAK");
    int lw = font3x5::textWidth(label, 1);
    font3x5::drawText(ctx.fb, label, (W - lw) / 2, 12, 1, accent);

    // draining ring around the timer
    int cx = W / 2, cy = H / 2 - 2, r = W / 2 - 14;
    float frac = 1.0f - float(elapsed) / float(phaseMs);
    RGB ringDim = accent;
    ringDim.dim(50);
    ctx.fb.circle(cx, cy, r, ringDim);
    int lit = int(64 * frac);
    for (int i = 0; i < lit; i++) {
      float a = -1.5708f + i * (TWO_PI_F / 64.0f);
      ctx.fb.set(cx + int(cosf(a) * r), cy + int(sinf(a) * r), accent);
      ctx.fb.set(cx + int(cosf(a) * (r - 1)), cy + int(sinf(a) * (r - 1)), accent);
    }

    // MM:SS big in the middle
    char t[8];
    snprintf(t, sizeof(t), "%d:%02d", mins, secs);
    int tw = font3x5::textWidth(t, 4);
    RGB digits = paused_ && (ctx.nowMs / 400) % 2 ? RGB(60, 60, 60) : RGB(240, 240, 240);
    font3x5::drawText(ctx.fb, t, (W - tw) / 2, cy - 10, 4, digits);

    // completed pomodoros as a row of dots
    for (int i = 0; i < pomodoros_ && i < 8; i++)
      ctx.fb.fillCircle(W / 2 - (pomodoros_ < 8 ? pomodoros_ : 8) * 3 + i * 6 + 2,
                        H - 8, 2, RGB(230, 70, 60));

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
