// Exercise guide. Where yoga holds a pose, exercise animates reps: each
// move tweens back and forth between two keyframes while a counter tracks
// reps, then rests and moves on. Two programs, chosen in settings:
// bodyweight and kettlebell (the bell is drawn in her hands).
#ifndef CORIOLIS_SCENE_EXERCISE_H
#define CORIOLIS_SCENE_EXERCISE_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/settings.h"
#include "guide_ui.h"
#include "figure.h"

namespace coriolis {

struct Exercise {
  const char* name;
  int face;
  bool kettlebell;
  float a[15][2];  // rep start/end
  float b[15][2];  // rep midpoint
};

namespace exercises {

// joint order: head neck lSho rSho lElb lWri rElb rWri pelvis
//              lKnee lAnk rKnee rAnk lToe rToe

// stand tall -> deep squat, arms reach forward for balance
static const Exercise SQUAT = {"SQUAT", 0, false,
  {{0.50f,0.18f},{0.50f,0.26f},{0.42f,0.29f},{0.58f,0.29f},{0.40f,0.42f},
   {0.39f,0.53f},{0.60f,0.42f},{0.61f,0.53f},{0.50f,0.52f},{0.46f,0.70f},
   {0.45f,0.90f},{0.54f,0.70f},{0.55f,0.90f},{0.41f,0.92f},{0.59f,0.92f}},
  {{0.50f,0.33f},{0.50f,0.40f},{0.43f,0.43f},{0.57f,0.43f},{0.34f,0.44f},
   {0.27f,0.43f},{0.66f,0.44f},{0.73f,0.43f},{0.50f,0.65f},{0.39f,0.74f},
   {0.42f,0.90f},{0.61f,0.74f},{0.58f,0.90f},{0.38f,0.92f},{0.62f,0.92f}}};

// arms/legs in -> arms up, legs wide (jumping jack)
static const Exercise JACKS = {"JACKS", 0, false,
  {{0.50f,0.15f},{0.50f,0.24f},{0.45f,0.27f},{0.55f,0.27f},{0.44f,0.40f},
   {0.44f,0.52f},{0.56f,0.40f},{0.56f,0.52f},{0.50f,0.51f},{0.49f,0.70f},
   {0.49f,0.90f},{0.51f,0.70f},{0.51f,0.90f},{0.46f,0.92f},{0.54f,0.92f}},
  {{0.50f,0.13f},{0.50f,0.22f},{0.43f,0.25f},{0.57f,0.25f},{0.32f,0.16f},
   {0.22f,0.08f},{0.68f,0.16f},{0.78f,0.08f},{0.50f,0.50f},{0.40f,0.68f},
   {0.30f,0.90f},{0.60f,0.68f},{0.70f,0.90f},{0.25f,0.92f},{0.75f,0.92f}}};

// kettlebell swing: hinge, bell between the legs -> stand, bell to shoulder
static const Exercise KB_SWING = {"KB SWING", -1, true,
  {{0.40f,0.30f},{0.44f,0.37f},{0.45f,0.39f},{0.46f,0.40f},{0.50f,0.50f},
   {0.55f,0.62f},{0.51f,0.50f},{0.56f,0.62f},{0.55f,0.52f},{0.60f,0.68f},
   {0.62f,0.90f},{0.61f,0.68f},{0.63f,0.90f},{0.56f,0.92f},{0.57f,0.92f}},
  {{0.42f,0.16f},{0.45f,0.24f},{0.45f,0.27f},{0.46f,0.28f},{0.42f,0.34f},
   {0.40f,0.42f},{0.43f,0.34f},{0.41f,0.42f},{0.50f,0.50f},{0.50f,0.70f},
   {0.50f,0.90f},{0.51f,0.70f},{0.51f,0.90f},{0.46f,0.92f},{0.47f,0.92f}}};

// goblet squat: bell at the chest, stand -> squat
static const Exercise KB_GOBLET = {"GOBLET", 0, true,
  {{0.50f,0.18f},{0.50f,0.26f},{0.43f,0.29f},{0.57f,0.29f},{0.45f,0.36f},
   {0.47f,0.40f},{0.55f,0.36f},{0.53f,0.40f},{0.50f,0.52f},{0.46f,0.70f},
   {0.45f,0.90f},{0.54f,0.70f},{0.55f,0.90f},{0.41f,0.92f},{0.59f,0.92f}},
  {{0.50f,0.33f},{0.50f,0.40f},{0.44f,0.43f},{0.56f,0.43f},{0.46f,0.49f},
   {0.48f,0.53f},{0.54f,0.49f},{0.52f,0.53f},{0.50f,0.65f},{0.39f,0.74f},
   {0.42f,0.90f},{0.61f,0.74f},{0.58f,0.90f},{0.38f,0.92f},{0.62f,0.92f}}};

// kettlebell press: bell at shoulder -> pressed overhead
static const Exercise KB_PRESS = {"KB PRESS", 0, true,
  {{0.50f,0.18f},{0.50f,0.26f},{0.42f,0.29f},{0.58f,0.29f},{0.40f,0.42f},
   {0.40f,0.53f},{0.62f,0.34f},{0.60f,0.28f},{0.50f,0.52f},{0.46f,0.70f},
   {0.45f,0.90f},{0.54f,0.70f},{0.55f,0.90f},{0.41f,0.92f},{0.59f,0.92f}},
  {{0.50f,0.18f},{0.50f,0.26f},{0.42f,0.29f},{0.58f,0.29f},{0.40f,0.42f},
   {0.40f,0.53f},{0.60f,0.16f},{0.60f,0.05f},{0.50f,0.52f},{0.46f,0.70f},
   {0.45f,0.90f},{0.54f,0.70f},{0.55f,0.90f},{0.41f,0.92f},{0.59f,0.92f}}};

}

class ExerciseScene : public Scene {
 public:
  explicit ExerciseScene(Settings& settings) : settings_(settings) {}

  const char* name() const { return "Exercise"; }
  bool autoplayEligible() const { return false; }

  void start(Context& ctx) {
    buildProgram();
    move_ = 0;
    resting_ = false;
    paused_ = false;
    phaseStartMs_ = ctx.nowMs;
  }

  bool input(Context&, Key k) {
    if (k == Key::Select) { paused_ = !paused_; return true; }
    if (k == Key::Up || k == Key::Down) {  // reps
      int r = settings_.exerciseReps + (k == Key::Up ? 2 : -2);
      if (r < 4) r = 4;
      if (r > 20) r = 20;
      settings_.exerciseReps = uint8_t(r);
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();
    buildProgram();  // reflect a program change made in settings

    uint32_t repMs = uint32_t(settings_.exerciseRepSec) * 1000;
    uint32_t elapsed = ctx.nowMs - phaseStartMs_;
    if (paused_) phaseStartMs_ = ctx.nowMs - elapsed;

    if (resting_) {
      int remain = int((REST_MS - elapsed) / 1000) + 1;
      if (!paused_ && elapsed >= REST_MS) {
        resting_ = false;
        move_ = (move_ + 1) % count_;
        phaseStartMs_ = ctx.nowMs;
        return 33;
      }
      drawRest(ctx, remain);
      return 33;
    }

    const Exercise& ex = *program_[move_];
    int rep = int(elapsed / repMs);
    if (!paused_ && rep >= settings_.exerciseReps) {
      resting_ = true;
      phaseStartMs_ = ctx.nowMs;
      // move complete: on the device a chime plays here
      return 33;
    }

    // triangle within a rep: a -> b -> a, eased
    float frac = (elapsed % repMs) / float(repMs);
    float tri = frac < 0.5f ? frac * 2.0f : (1.0f - frac) * 2.0f;
    tri = tri * tri * (3.0f - 2.0f * tri);

    float j[15][2];
    guide::tweenPose(ex.a, ex.b, tri, j);

    guide::drawFigure(ctx, j, ex.face, settings_.yogaBody);
    if (ex.kettlebell) guide::drawKettlebell(ctx, j);

    guide::drawTopBar(ctx, ex.name);

    char counter[8];
    snprintf(counter, sizeof(counter), "%d.%d", rep + 1,
             settings_.exerciseReps);
    font3x5::drawText(ctx.fb, counter, ctx.fb.width() - 18, 2, 1,
                      guide::mutedColor());

    if (paused_) {
      ctx.fb.rect(ctx.fb.width() - 28, 2, 2, 5, guide::titleColor());
      ctx.fb.rect(ctx.fb.width() - 24, 2, 2, 5, guide::titleColor());
    }

    // rep progress along the bottom
    int barW = int(ctx.fb.width() * (elapsed % repMs) / repMs);
    ctx.fb.hLine(0, barW, ctx.fb.height() - 1, guide::matColor());

    return 33;
  }

 private:
  static const uint32_t REST_MS = 4000;
  static const int MAX_MOVES = 4;

  Settings& settings_;
  const Exercise* program_[MAX_MOVES];
  int count_ = 0;
  int move_ = 0;
  bool resting_ = false;
  bool paused_ = false;
  uint32_t phaseStartMs_ = 0;

  void buildProgram() {
    if (settings_.exerciseProgram == 0) {
      program_[0] = &exercises::SQUAT;
      program_[1] = &exercises::JACKS;
      count_ = 2;
    } else {
      program_[0] = &exercises::KB_SWING;
      program_[1] = &exercises::KB_GOBLET;
      program_[2] = &exercises::KB_PRESS;
      count_ = 3;
    }
    if (move_ >= count_) move_ = 0;
  }

  void drawRest(Context& ctx, int remain) {
    guide::drawTopBar(ctx, "REST");
    char n[4];
    snprintf(n, sizeof(n), "%d", remain < 1 ? 1 : remain);
    int w = font3x5::textWidth(n, 5);
    font3x5::drawText(ctx.fb, n, (ctx.fb.width() - w) / 2,
                      ctx.fb.height() / 2 - 12, 5, guide::titleColor());

    const Exercise& next = *program_[(move_ + 1) % count_];
    char line[20];
    snprintf(line, sizeof(line), "NEXT %s", next.name);
    int lw = font3x5::textWidth(line, 1);
    font3x5::drawText(ctx.fb, line, (ctx.fb.width() - lw) / 2,
                      ctx.fb.height() - 16, 1, guide::mutedColor());
  }
};

}

#endif
