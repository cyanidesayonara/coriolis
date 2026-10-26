// Exercise guide. Where yoga holds a pose, exercise animates reps: each
// move tweens back and forth between two keyframes while a counter tracks
// reps, then rests (previewing the next move) and moves on. Four programs,
// picked on the setup card or in settings: bodyweight, kettlebell (the bell
// is drawn in her hands), Tabata HIIT (8 x 20s on / 10s off, framed green
// and amber so the phase reads from across the room) and core. One-sided
// moves run right then left, mirrored. Sessions end on a summary card.
#ifndef CORIOLIS_SCENE_EXERCISE_H
#define CORIOLIS_SCENE_EXERCISE_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/settings.h"
#include "guide_ui.h"
#include "figure.h"
#include "intro.h"
#include "../core/audio.h"
#include "../core/font5x7.h"

namespace coriolis {

struct Exercise {
  const char* name;
  int voice;  // voice::MOVE_BASE + this (see docs/AUDIO.md)
  int face;
  bool kettlebell;
  float a[15][2];  // rep start/end
  float b[15][2];  // rep midpoint
};

namespace exercises {

// joint order: head neck lSho rSho lElb lWri rElb rWri pelvis
//              lKnee lAnk rKnee rAnk lToe rToe

// stand tall -> deep squat, arms reach forward for balance
static const Exercise SQUAT = {"SQUAT", 1, 0, false,
  {{0.50f,0.18f},{0.50f,0.26f},{0.42f,0.29f},{0.58f,0.29f},{0.40f,0.42f},
   {0.39f,0.53f},{0.60f,0.42f},{0.61f,0.53f},{0.50f,0.52f},{0.46f,0.70f},
   {0.45f,0.90f},{0.54f,0.70f},{0.55f,0.90f},{0.41f,0.92f},{0.59f,0.92f}},
  {{0.50f,0.33f},{0.50f,0.40f},{0.43f,0.43f},{0.57f,0.43f},{0.34f,0.44f},
   {0.27f,0.43f},{0.66f,0.44f},{0.73f,0.43f},{0.50f,0.65f},{0.39f,0.74f},
   {0.42f,0.90f},{0.61f,0.74f},{0.58f,0.90f},{0.38f,0.92f},{0.62f,0.92f}}};

// arms/legs in -> arms up, legs wide (jumping jack)
static const Exercise JACKS = {"JACKS", 5, 0, false,
  {{0.50f,0.15f},{0.50f,0.24f},{0.45f,0.27f},{0.55f,0.27f},{0.44f,0.40f},
   {0.44f,0.52f},{0.56f,0.40f},{0.56f,0.52f},{0.50f,0.51f},{0.49f,0.70f},
   {0.49f,0.90f},{0.51f,0.70f},{0.51f,0.90f},{0.46f,0.92f},{0.54f,0.92f}},
  {{0.50f,0.13f},{0.50f,0.22f},{0.43f,0.25f},{0.57f,0.25f},{0.32f,0.16f},
   {0.22f,0.08f},{0.68f,0.16f},{0.78f,0.08f},{0.50f,0.50f},{0.40f,0.68f},
   {0.30f,0.90f},{0.60f,0.68f},{0.70f,0.90f},{0.25f,0.92f},{0.75f,0.92f}}};

// pushup (side view): a = plank on straight arms, b = chest lowered
static const Exercise PUSHUP = {"PUSHUP", 2, -1, false,
  {{0.26f,0.48f},{0.31f,0.55f},{0.32f,0.57f},{0.33f,0.58f},{0.31f,0.72f},
   {0.30f,0.90f},{0.32f,0.73f},{0.31f,0.90f},{0.52f,0.66f},{0.64f,0.73f},
   {0.76f,0.81f},{0.65f,0.74f},{0.77f,0.82f},{0.80f,0.90f},{0.81f,0.91f}},
  {{0.25f,0.70f},{0.30f,0.76f},{0.31f,0.78f},{0.32f,0.79f},{0.38f,0.76f},
   {0.30f,0.90f},{0.39f,0.77f},{0.31f,0.90f},{0.52f,0.79f},{0.64f,0.81f},
   {0.76f,0.84f},{0.65f,0.82f},{0.77f,0.85f},{0.80f,0.91f},{0.81f,0.92f}}};

// lunge (side view): a = standing tall, b = deep lunge, back knee dipping
static const Exercise LUNGE = {"LUNGE", 4, -1, false,
  {{0.48f,0.24f},{0.50f,0.32f},{0.50f,0.34f},{0.51f,0.35f},{0.50f,0.46f},
   {0.49f,0.56f},{0.51f,0.46f},{0.50f,0.56f},{0.52f,0.56f},{0.51f,0.72f},
   {0.50f,0.90f},{0.52f,0.72f},{0.51f,0.90f},{0.44f,0.92f},{0.45f,0.92f}},
  {{0.48f,0.32f},{0.50f,0.40f},{0.50f,0.42f},{0.51f,0.43f},{0.49f,0.52f},
   {0.51f,0.60f},{0.52f,0.53f},{0.53f,0.60f},{0.52f,0.68f},{0.38f,0.74f},
   {0.36f,0.90f},{0.62f,0.84f},{0.70f,0.86f},{0.30f,0.92f},{0.76f,0.91f}}};

// situp (side view): a = lying with the knees bent, b = crunched up toward them
static const Exercise SITUP = {"SITUP", 3, -1, false,
  {{0.24f,0.84f},{0.30f,0.85f},{0.32f,0.86f},{0.33f,0.87f},{0.38f,0.82f},
   {0.44f,0.78f},{0.39f,0.82f},{0.45f,0.78f},{0.52f,0.88f},{0.62f,0.72f},
   {0.66f,0.88f},{0.63f,0.73f},{0.67f,0.88f},{0.72f,0.90f},{0.73f,0.91f}},
  {{0.38f,0.60f},{0.41f,0.68f},{0.42f,0.70f},{0.43f,0.71f},{0.48f,0.72f},
   {0.54f,0.70f},{0.49f,0.73f},{0.55f,0.70f},{0.52f,0.88f},{0.62f,0.72f},
   {0.66f,0.88f},{0.63f,0.73f},{0.67f,0.88f},{0.72f,0.90f},{0.73f,0.91f}}};

// kettlebell row (side view): hinged flat back; a = bell hanging,
// b = elbows pulled up past the ribs
static const Exercise KB_ROW = {"KB ROW", 8, -1, true,
  {{0.36f,0.46f},{0.41f,0.49f},{0.43f,0.51f},{0.45f,0.52f},{0.44f,0.64f},
   {0.45f,0.78f},{0.46f,0.64f},{0.47f,0.78f},{0.58f,0.62f},{0.48f,0.72f},
   {0.50f,0.90f},{0.51f,0.72f},{0.53f,0.90f},{0.45f,0.92f},{0.48f,0.92f}},
  {{0.36f,0.46f},{0.41f,0.49f},{0.43f,0.51f},{0.45f,0.52f},{0.52f,0.52f},
   {0.48f,0.62f},{0.54f,0.53f},{0.50f,0.62f},{0.58f,0.62f},{0.48f,0.72f},
   {0.50f,0.90f},{0.51f,0.72f},{0.53f,0.90f},{0.45f,0.92f},{0.48f,0.92f}}};

// kettlebell swing (side view, facing left): a = gorilla hinge with the
// bell hanging near the floor between the legs; b = stand tall with the
// arms swung forward and up, bell arcing outward to head height
static const Exercise KB_SWING = {"KB SWING", 6, -1, true,
  {{0.36f,0.46f},{0.41f,0.49f},{0.43f,0.51f},{0.45f,0.52f},{0.44f,0.66f},
   {0.45f,0.83f},{0.46f,0.66f},{0.47f,0.83f},{0.58f,0.62f},{0.48f,0.72f},
   {0.50f,0.90f},{0.51f,0.72f},{0.53f,0.90f},{0.45f,0.92f},{0.48f,0.92f}},
  {{0.50f,0.17f},{0.50f,0.25f},{0.49f,0.29f},{0.51f,0.30f},{0.41f,0.24f},
   {0.33f,0.20f},{0.42f,0.25f},{0.34f,0.21f},{0.51f,0.52f},{0.50f,0.71f},
   {0.50f,0.90f},{0.52f,0.71f},{0.52f,0.90f},{0.45f,0.92f},{0.47f,0.92f}}};

// goblet squat: bell at the chest, stand -> squat
static const Exercise KB_GOBLET = {"GOBLET", 7, 0, true,
  {{0.50f,0.18f},{0.50f,0.26f},{0.43f,0.29f},{0.57f,0.29f},{0.45f,0.36f},
   {0.47f,0.40f},{0.55f,0.36f},{0.53f,0.40f},{0.50f,0.52f},{0.46f,0.70f},
   {0.45f,0.90f},{0.54f,0.70f},{0.55f,0.90f},{0.41f,0.92f},{0.59f,0.92f}},
  {{0.50f,0.33f},{0.50f,0.40f},{0.44f,0.43f},{0.56f,0.43f},{0.46f,0.49f},
   {0.48f,0.53f},{0.54f,0.49f},{0.52f,0.53f},{0.50f,0.65f},{0.39f,0.74f},
   {0.42f,0.90f},{0.61f,0.74f},{0.58f,0.90f},{0.38f,0.92f},{0.62f,0.92f}}};

// kettlebell press (two-handed, front view): both hands hold the bell at
// the chest (a) and press it straight overhead (b). Both wrists share a
// point so the bell renders at the hands, not in the body's center.
static const Exercise KB_PRESS = {"KB PRESS", 9, 0, true,
  {{0.50f,0.18f},{0.50f,0.26f},{0.42f,0.29f},{0.58f,0.29f},{0.45f,0.37f},
   {0.48f,0.33f},{0.55f,0.37f},{0.52f,0.33f},{0.50f,0.52f},{0.46f,0.70f},
   {0.45f,0.90f},{0.54f,0.70f},{0.55f,0.90f},{0.41f,0.92f},{0.59f,0.92f}},
  {{0.50f,0.18f},{0.50f,0.26f},{0.42f,0.29f},{0.58f,0.29f},{0.46f,0.14f},
   {0.48f,0.05f},{0.54f,0.14f},{0.52f,0.05f},{0.50f,0.52f},{0.46f,0.70f},
   {0.45f,0.90f},{0.54f,0.70f},{0.55f,0.90f},{0.41f,0.92f},{0.59f,0.92f}}};

// glute bridge (side view): a = lying with the knees bent, b = hips lifted
static const Exercise GLUTE_BRIDGE = {"BRIDGE", 13, -1, false,
  {{0.24f,0.86f},{0.30f,0.86f},{0.32f,0.87f},{0.33f,0.88f},{0.40f,0.89f},
   {0.48f,0.90f},{0.41f,0.89f},{0.49f,0.90f},{0.54f,0.86f},{0.64f,0.70f},
   {0.68f,0.88f},{0.65f,0.71f},{0.69f,0.88f},{0.74f,0.90f},{0.75f,0.91f}},
  {{0.24f,0.88f},{0.30f,0.86f},{0.32f,0.87f},{0.33f,0.88f},{0.40f,0.89f},
   {0.48f,0.90f},{0.41f,0.89f},{0.49f,0.90f},{0.52f,0.70f},{0.64f,0.66f},
   {0.68f,0.88f},{0.65f,0.67f},{0.69f,0.88f},{0.74f,0.90f},{0.75f,0.91f}}};

// plank hold (side view): a long straight line; b only breathes
static const Exercise PLANK_HOLD = {"PLANK", 14, -1, false,
  {{0.26f,0.50f},{0.31f,0.56f},{0.32f,0.57f},{0.33f,0.58f},{0.31f,0.73f},
   {0.30f,0.90f},{0.32f,0.74f},{0.31f,0.90f},{0.52f,0.67f},{0.64f,0.74f},
   {0.76f,0.82f},{0.65f,0.75f},{0.77f,0.83f},{0.80f,0.90f},{0.81f,0.91f}},
  {{0.26f,0.51f},{0.31f,0.57f},{0.32f,0.58f},{0.33f,0.59f},{0.31f,0.73f},
   {0.30f,0.90f},{0.32f,0.74f},{0.31f,0.90f},{0.52f,0.68f},{0.64f,0.74f},
   {0.76f,0.82f},{0.65f,0.75f},{0.77f,0.83f},{0.80f,0.90f},{0.81f,0.91f}}};

// mountain climbers (side view): from plank, knees drive to the chest in turn
static const Exercise CLIMBER = {"CLIMBERS", 15, -1, false,
  {{0.26f,0.50f},{0.31f,0.56f},{0.32f,0.57f},{0.33f,0.58f},{0.31f,0.73f},
   {0.30f,0.90f},{0.32f,0.74f},{0.31f,0.90f},{0.52f,0.65f},{0.42f,0.76f},
   {0.50f,0.85f},{0.65f,0.74f},{0.77f,0.83f},{0.47f,0.89f},{0.81f,0.91f}},
  {{0.26f,0.50f},{0.31f,0.56f},{0.32f,0.57f},{0.33f,0.58f},{0.31f,0.73f},
   {0.30f,0.90f},{0.32f,0.74f},{0.31f,0.90f},{0.52f,0.65f},{0.64f,0.74f},
   {0.76f,0.83f},{0.42f,0.76f},{0.50f,0.85f},{0.80f,0.91f},{0.47f,0.89f}}};

// high knees (front view): running on the spot, arms pumping opposite
static const Exercise HIGH_KNEES = {"HIGH KNEES", 16, 0, false,
  {{0.50f,0.18f},{0.50f,0.26f},{0.42f,0.29f},{0.58f,0.29f},{0.37f,0.38f},
   {0.40f,0.30f},{0.62f,0.40f},{0.62f,0.50f},{0.50f,0.52f},{0.45f,0.60f},
   {0.45f,0.75f},{0.54f,0.70f},{0.55f,0.90f},{0.42f,0.76f},{0.59f,0.92f}},
  {{0.50f,0.18f},{0.50f,0.26f},{0.42f,0.29f},{0.58f,0.29f},{0.38f,0.40f},
   {0.38f,0.50f},{0.63f,0.38f},{0.60f,0.30f},{0.50f,0.52f},{0.46f,0.70f},
   {0.45f,0.90f},{0.55f,0.60f},{0.55f,0.75f},{0.41f,0.92f},{0.58f,0.76f}}};

}

class ExerciseScene : public Scene {
 public:
  explicit ExerciseScene(Settings& settings) : settings_(settings) {}

  static const int PROGRAM_COUNT = 4;

  const char* name() const { return "Exercise"; }
  bool autoplayEligible() const { return false; }

  static const char* programName(int i) {
    static const char* n[PROGRAM_COUNT] = {"Bodyweight", "Kettlebell",
                                           "Tabata HIIT", "Core"};
    return n[(i < 0 || i >= PROGRAM_COUNT) ? 0 : i];
  }

  void start(Context&) {
    phase_ = SETUP;
    buildProgram();
  }

  bool input(Context& ctx, Key k) {
    switch (phase_) {
      case SETUP:
        if (k == Key::Select) { begin(ctx); return true; }
        if (k == Key::Up || k == Key::Down) {  // pick the program
          int d = k == Key::Up ? 1 : PROGRAM_COUNT - 1;
          settings_.exerciseProgram =
              uint8_t((settings_.exerciseProgram + d) % PROGRAM_COUNT);
          buildProgram();
          return true;
        }
        return false;
      case COUNTDOWN:
        if (k == Key::Select) { enterStep(ctx, 0); return true; }
        if (k == Key::Back) { phase_ = SETUP; return true; }
        return true;
      case RUN:
        if (k == Key::Select) { paused_ = !paused_; return true; }
        if ((k == Key::Up || k == Key::Down) && !tabata()) {  // reps
          int r = settings_.exerciseReps + (k == Key::Up ? 2 : -2);
          settings_.exerciseReps = uint8_t(r < 4 ? 4 : (r > 20 ? 20 : r));
          return true;
        }
        if (k == Key::Up || k == Key::Down) return true;
        if (k == Key::Right) { advance(ctx); return true; }
        if (k == Key::Left) {  // back to the previous move (or redo this one)
          int i = step_ - 1;
          while (i > 0 && steps_[i].rest) i--;
          enterStep(ctx, i < 0 ? 0 : i);
          return true;
        }
        if (k == Key::Back) { phase_ = SETUP; return true; }
        return false;
      case DONE:
        if (k == Key::Select) { begin(ctx); return true; }
        if (k == Key::Back) { phase_ = SETUP; return true; }
        return false;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    if (phase_ == SETUP) {
      if (settings_.exerciseProgram != lastProgram_) buildProgram();
      char l0[24], l1[24];
      snprintf(l0, sizeof(l0), "%s", programName(settings_.exerciseProgram));
      if (tabata())
        snprintf(l1, sizeof(l1), "8 x 20s on, 10s off");
      else
        snprintf(l1, sizeof(l1), "%d moves, %d reps", moves_ % 100,
                 settings_.exerciseReps % 100);
      const char* lines[] = {l0, l1};
      intro::draw(ctx, "Exercise", lines, 2, guide::titleColor());
      return 33;
    }

    if (phase_ == COUNTDOWN) {
      uint32_t e = ctx.nowMs - phaseStartMs_;
      int sec = int(e / 1000);
      if (sec != lastTick_ && sec < 3) {
        lastTick_ = sec;
        ctx.audio.play(Cue::Tick);
      }
      if (e < COUNTDOWN_MS) {
        guide::drawCountdown(ctx, e, COUNTDOWN_MS, steps_[0].ex->name);
        return 33;
      }
      enterStep(ctx, 0);
    }

    if (phase_ == DONE) {
      char stats[24], dur[8];
      guide::formatDuration(sessionMs_, dur, sizeof(dur));
      snprintf(stats, sizeof(stats), "%d moves  %s", moves_ % 100, dur);
      guide::drawCompletion(ctx, "Great job!", stats, ctx.nowMs - phaseStartMs_);
      return 33;
    }

    ctx.fb.clear();
    const Step& st = steps_[step_];
    uint32_t elapsed = ctx.nowMs - stepStartMs_;
    if (paused_) {
      stepStartMs_ = ctx.nowMs - frozenElapsed_;
      elapsed = frozenElapsed_;
    } else {
      frozenElapsed_ = elapsed;
      sessionMs_ += ctx.nowMs - lastDrawMs_;
    }
    lastDrawMs_ = ctx.nowMs;

    uint32_t len = stepLength(st);
    if (!paused_ && elapsed >= len) {
      advance(ctx);
      return 33;
    }

    // halfway chime, and ticks through the last three seconds of anything
    // timed (rests, holds, intervals)
    if (!paused_) {
      if (!halfwayDone_ && !st.rest && elapsed >= len / 2 && len >= 10000) {
        halfwayDone_ = true;
        ctx.audio.play(Cue::Halfway);
      }
      int left = int((len - elapsed + 999) / 1000);
      if ((st.rest || st.timed) && left <= 3 && left != lastTick_) {
        lastTick_ = left;
        ctx.audio.play(Cue::Tick);
      }
    }

    if (st.rest) drawRest(ctx, st, len, elapsed);
    else drawWork(ctx, st, len, elapsed);

    if (tabata()) {  // full-screen interval frame: green go, amber breathe
      RGB c = st.rest ? guide::restColor() : guide::workColor();
      c.dim(paused_ ? 90 : 200);
      const int W = ctx.fb.width(), H = ctx.fb.height();
      ctx.fb.rect(0, 0, W, 1, c);
      ctx.fb.rect(0, H - 2, W, 1, c);
      ctx.fb.rect(0, 0, 1, H - 1, c);
      ctx.fb.rect(W - 1, 0, 1, H - 1, c);
    }

    if (paused_) guide::drawPaused(ctx);

    // step progress along the bottom
    int barW = int(uint64_t(ctx.fb.width()) * elapsed / (len ? len : 1));
    ctx.fb.hLine(0, barW, ctx.fb.height() - 1,
                 st.rest ? guide::restColor() : guide::matColor());
    return 33;
  }

 private:
  static const int MAX_STEPS = 24;
  static const uint32_t COUNTDOWN_MS = 3000;
  static const uint32_t REST_MS = 10000;
  static const uint32_t SWITCH_MS = 5000;

  enum Phase { SETUP, COUNTDOWN, RUN, DONE };
  struct Step {
    const Exercise* ex;  // for a rest: the move coming up next
    int8_t side;         // 0 both, +1 right, -1 left (mirrored)
    bool rest;
    bool timed;          // a hold or interval instead of counted reps
    uint16_t seconds;    // timed steps and rests
  };

  Settings& settings_;
  Step steps_[MAX_STEPS];
  int count_ = 0;
  int moves_ = 0;  // distinct moves, for the setup card and summary
  int step_ = 0;
  uint8_t lastProgram_ = 255;
  Phase phase_ = SETUP;
  bool paused_ = false;
  bool halfwayDone_ = false;
  int lastRep_ = -1;
  int lastTick_ = -1;
  uint32_t repFlashMs_ = 0;
  uint32_t stepStartMs_ = 0;
  uint32_t phaseStartMs_ = 0;
  uint32_t lastDrawMs_ = 0;
  uint32_t frozenElapsed_ = 0;
  uint32_t sessionMs_ = 0;

  bool tabata() const { return settings_.exerciseProgram == 2; }

  uint32_t repMs() const {
    return tabata() ? 900 : uint32_t(settings_.exerciseRepSec) * 1000;
  }

  uint32_t stepLength(const Step& st) const {
    if (st.rest || st.timed) return uint32_t(st.seconds) * 1000;
    return repMs() * settings_.exerciseReps;
  }

  void begin(Context& ctx) {
    buildProgram();
    phase_ = COUNTDOWN;
    phaseStartMs_ = ctx.nowMs;
    lastTick_ = -1;
    sessionMs_ = 0;
    ctx.audio.voice(voice::GET_READY);
  }

  void enterStep(Context& ctx, int i) {
    if (phase_ != RUN) {
      phase_ = RUN;
      ctx.audio.play(Cue::StartBell);
    }
    int prev = step_;
    step_ = i;
    stepStartMs_ = ctx.nowMs;
    lastDrawMs_ = ctx.nowMs;
    frozenElapsed_ = 0;
    paused_ = false;
    halfwayDone_ = false;
    lastRep_ = -1;
    lastTick_ = -1;
    const Step& st = steps_[i];
    if (st.rest) {
      if (tabata()) ctx.audio.voice(voice::REST);
      return;
    }
    ctx.audio.play(Cue::Chime);
    // same move again on the other side: say so instead of its name
    int last = prev;
    while (last > 0 && steps_[last].rest) last--;
    bool otherSide = i > 0 && last != i && steps_[last].ex == st.ex &&
                     steps_[last].side == -st.side && st.side != 0;
    ctx.audio.voice(otherSide ? voice::SWITCH_SIDES
                              : voice::MOVE_BASE + st.ex->voice);
  }

  void advance(Context& ctx) {
    if (step_ + 1 >= count_) {
      phase_ = DONE;
      phaseStartMs_ = ctx.nowMs;
      ctx.audio.play(Cue::FinishBell);
      ctx.audio.voice(voice::COMPLETE);
      return;
    }
    if (!steps_[step_].rest) ctx.audio.play(Cue::FinishBell);  // move done
    enterStep(ctx, step_ + 1);
  }

  void pose(const Step& st, float tri, float j[15][2], int& face) const {
    float a[15][2], b[15][2];
    if (st.side < 0) {
      guide::mirrorPose(st.ex->a, a);
      guide::mirrorPose(st.ex->b, b);
      face = -st.ex->face;
    } else {
      for (int k = 0; k < 15; k++) {
        a[k][0] = st.ex->a[k][0]; a[k][1] = st.ex->a[k][1];
        b[k][0] = st.ex->b[k][0]; b[k][1] = st.ex->b[k][1];
      }
      face = st.ex->face;
    }
    guide::tweenPose(a, b, tri, j);
  }

  static float triangle(uint32_t t, uint32_t period) {
    float frac = (t % period) / float(period);
    float tri = frac < 0.5f ? frac * 2.0f : (1.0f - frac) * 2.0f;
    return tri * tri * (3.0f - 2.0f * tri);
  }

  void drawWork(Context& ctx, const Step& st, uint32_t len, uint32_t elapsed) {
    const Exercise& ex = *st.ex;
    uint32_t rm = repMs();
    int rep = int(elapsed / rm);

    // count the reps aloud, like a coach (not during timed work)
    if (!st.timed && !paused_ && rep != lastRep_ && rep < settings_.exerciseReps) {
      lastRep_ = rep;
      repFlashMs_ = ctx.nowMs;
      ctx.audio.voice(voice::NUMBER_BASE + rep + 1);
    }

    // holds only breathe; everything else runs a -> b -> a each rep
    float j[15][2];
    int face;
    pose(st, triangle(elapsed, st.timed && !tabata() ? 4000 : rm), j, face);
    guide::drawFigure(ctx, j, face, settings_.yogaBody);
    if (ex.kettlebell) guide::drawKettlebell(ctx, j);

    // a ring pops outward from the body on each counted rep
    uint32_t sinceRep = ctx.nowMs - repFlashMs_;
    if (!st.timed && sinceRep < 240) {
      int cx = ctx.fb.width() / 2, cy = ctx.fb.height() / 2;
      RGB ring = guide::titleColor();
      ring.dim(uint8_t(220 - sinceRep));
      ctx.fb.circle(cx, cy, 6 + int(sinceRep / 12), ring);
    }

    guide::drawTopBar(ctx, ex.name);
    guide::drawSideTag(ctx, st.side);

    char counter[12];
    if (tabata())
      snprintf(counter, sizeof(counter), "R%d.8", (roundOf(step_) + 1) % 10);
    else if (st.timed)
      snprintf(counter, sizeof(counter), "%ds", int((len - elapsed + 999) / 1000) % 1000);
    else
      snprintf(counter, sizeof(counter), "%d.%d", (rep + 1) % 100,
               settings_.exerciseReps % 100);
    font3x5::drawText(ctx.fb, counter,
                      ctx.fb.width() - font3x5::textWidth(counter, 1) - 2, 2, 1,
                      guide::mutedColor());

    // timed work: big seconds remaining, top-right under the bar
    if (st.timed) {
      char secs[4];
      snprintf(secs, sizeof(secs), "%d", int((len - elapsed + 999) / 1000) % 100);
      RGB c = tabata() ? guide::workColor() : guide::titleColor();
      int w = font5x7::textWidth(secs, 2);
      font5x7::drawText(ctx.fb, secs, ctx.fb.width() - w - 4, 12, 2, c);
    }
  }

  void drawRest(Context& ctx, const Step& st, uint32_t len, uint32_t elapsed) {
    const int W = ctx.fb.width(), H = ctx.fb.height();
    bool sideSwitch = step_ > 0 && !steps_[step_ - 1].rest &&
                      steps_[step_ - 1].ex == st.ex && st.side != 0;
    guide::drawTopBar(ctx, sideSwitch ? "SWITCH SIDES" : "REST");

    int left = int((len - elapsed + 999) / 1000);
    char n[4];
    snprintf(n, sizeof(n), "%d", left % 100);
    RGB top = tabata() ? guide::restColor() : guide::titleColor();
    RGB bottom = top;
    bottom.dim(150);
    int scale = H >= 100 ? 4 : 2;
    font5x7::drawText(ctx.fb, n, 8, 18, scale, top, bottom);

    // the move coming up, previewed small and already moving
    if (H >= 100) {
      int span = 62;
      int ox = W - span - 4, oy = 14;
      float j[15][2];
      int face;
      pose(st, triangle(ctx.nowMs, 2400), j, face);
      guide::drawFigureAt(ctx, j, face, settings_.yogaBody, ox, oy, span, false);
      if (st.ex->kettlebell) guide::drawKettlebellAt(ctx, j, ox, oy, span);
    }

    char line[28];
    snprintf(line, sizeof(line), "Next: %s", st.ex->name);
    for (char* p = line + 6; *p; ++p)  // "KB SWING" -> "Kb swing" reads softer
      if (*p >= 'A' && *p <= 'Z' && p != line + 6 && p[-1] != ' ') *p = char(*p + 32);
    font5x7::drawCentered(ctx.fb, line, W / 2, H - 22, 1, RGB(220, 220, 228));
    if (st.side != 0) {
      const char* s = st.side > 0 ? "right side" : "left side";
      font3x5::drawText(ctx.fb, s, (W - font3x5::textWidth(s, 1)) / 2, H - 12,
                        1, guide::mutedColor());
    }
  }

  int roundOf(int i) const {
    int r = 0;
    for (int k = 0; k < i; k++)
      if (!steps_[k].rest) r++;
    return r;
  }

  void addMove(const Exercise& ex, bool bilateral, bool timed = false,
               uint16_t seconds = 0) {
    for (int side = 0; side < (bilateral ? 2 : 1); side++) {
      if (count_ + 2 > MAX_STEPS) return;
      int8_t s = bilateral ? int8_t(side == 0 ? 1 : -1) : int8_t(0);
      if (count_ > 0) {  // a rest before every move: longer between moves
        Step& r = steps_[count_++];
        r.ex = &ex; r.side = s; r.rest = true; r.timed = true;
        r.seconds = uint16_t((side == 1 ? SWITCH_MS : REST_MS) / 1000);
      }
      Step& w = steps_[count_++];
      w.ex = &ex; w.side = s; w.rest = false; w.timed = timed; w.seconds = seconds;
    }
    moves_++;
  }

  void buildProgram() {
    using namespace exercises;
    lastProgram_ = settings_.exerciseProgram;
    count_ = 0;
    moves_ = 0;
    switch (settings_.exerciseProgram) {
      case 1:  // kettlebell full body
        addMove(KB_SWING, false); addMove(KB_GOBLET, false);
        addMove(KB_ROW, true); addMove(KB_PRESS, false);
        break;
      case 2: {  // tabata: 8 x (20s work, 10s rest), four moves twice over
        const Exercise* cycle[4] = {&HIGH_KNEES, &CLIMBER, &SQUAT, &JACKS};
        for (int r = 0; r < 8; r++) {
          if (count_ > 0) {
            Step& rs = steps_[count_++];
            rs.ex = cycle[r % 4]; rs.side = 0; rs.rest = true; rs.timed = true;
            rs.seconds = 10;
          }
          Step& w = steps_[count_++];
          w.ex = cycle[r % 4]; w.side = 0; w.rest = false; w.timed = true;
          w.seconds = 20;
        }
        moves_ = 8;
        break;
      }
      case 3:  // core
        addMove(SITUP, false); addMove(GLUTE_BRIDGE, false);
        addMove(PUSHUP, false); addMove(PLANK_HOLD, false, true, 30);
        addMove(CLIMBER, false);
        break;
      default:  // bodyweight circuit
        addMove(SQUAT, false); addMove(PUSHUP, false); addMove(LUNGE, true);
        addMove(SITUP, false); addMove(JACKS, false); addMove(GLUTE_BRIDGE, false);
        break;
    }
    if (step_ >= count_) step_ = 0;
  }
};

}

#endif
