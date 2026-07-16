// Yoga routine player. Poses are skeletons — 15 joints in a normalized
// 0..1 space — so a pose is data, transitions tween smoothly between
// keyframes, and the figure scales to any display size. Toes are part of
// the pose: down dog holds the feet flexed (heels reaching, toes toward
// the hands) while cobra points them (tops of the feet on the floor).
//
// Guide scenes keep their own fixed look (guide_ui.h) — only the top-bar
// clock follows the theme palette.
#ifndef CORIOLIS_SCENE_YOGA_H
#define CORIOLIS_SCENE_YOGA_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"
#include "../core/settings.h"
#include "guide_ui.h"
#include "figure.h"
#include "intro.h"

namespace coriolis {

struct YogaPose {
  const char* name;
  int face;  // -1 profile left, 0 front, +1 profile right
  // x,y pairs, 0..1: head, neck, lSho, rSho, lElb, lWri, rElb, rWri,
  // pelvis, lKnee, lAnk, rKnee, rAnk, lToe, rToe
  float j[15][2];
};

namespace yoga_poses {

// front view, standing straight, arms at the sides, feet grounded
static const YogaPose MOUNTAIN = {"MOUNTAIN", 0, {
    {0.50f, 0.185f}, {0.50f, 0.26f}, {0.42f, 0.29f}, {0.58f, 0.29f},
    {0.40f, 0.42f}, {0.39f, 0.54f}, {0.60f, 0.42f}, {0.61f, 0.54f},
    {0.50f, 0.53f}, {0.46f, 0.70f}, {0.45f, 0.90f}, {0.54f, 0.70f},
    {0.55f, 0.90f}, {0.41f, 0.92f}, {0.59f, 0.92f}}};

// front view, legs wide, arms raised in a V
static const YogaPose STAR = {"STAR", 0, {
    {0.50f, 0.19f}, {0.50f, 0.27f}, {0.42f, 0.30f}, {0.58f, 0.30f},
    {0.30f, 0.21f}, {0.20f, 0.11f}, {0.70f, 0.21f}, {0.80f, 0.11f},
    {0.50f, 0.53f}, {0.38f, 0.70f}, {0.28f, 0.90f}, {0.62f, 0.70f},
    {0.72f, 0.90f}, {0.23f, 0.92f}, {0.77f, 0.92f}}};

// front view, right foot against the standing leg, palms together overhead
static const YogaPose TREE = {"TREE", 0, {
    {0.50f, 0.165f}, {0.50f, 0.24f}, {0.43f, 0.27f}, {0.57f, 0.27f},
    {0.37f, 0.16f}, {0.48f, 0.05f}, {0.63f, 0.16f}, {0.52f, 0.05f},
    {0.50f, 0.52f}, {0.47f, 0.70f}, {0.46f, 0.90f}, {0.63f, 0.62f},
    {0.51f, 0.68f}, {0.42f, 0.92f}, {0.50f, 0.73f}}};

// wide lunge, arms straight out over the legs
static const YogaPose WARRIOR = {"WARRIOR 2", 0, {
    {0.50f, 0.225f}, {0.50f, 0.30f}, {0.42f, 0.33f}, {0.58f, 0.33f},
    {0.28f, 0.33f}, {0.13f, 0.33f}, {0.72f, 0.33f}, {0.87f, 0.33f},
    {0.50f, 0.56f}, {0.31f, 0.70f}, {0.28f, 0.90f}, {0.66f, 0.73f},
    {0.75f, 0.90f}, {0.21f, 0.91f}, {0.82f, 0.91f}}};

// side view, the inverted V: hands planted, hips high, heels reaching —
// feet flexed, toes toward the hands
static const YogaPose DOWN_DOG = {"DOWN DOG", -1, {
    {0.32f, 0.575f}, {0.36f, 0.52f}, {0.34f, 0.54f}, {0.36f, 0.55f},
    {0.26f, 0.70f}, {0.19f, 0.90f}, {0.28f, 0.71f}, {0.21f, 0.91f},
    {0.56f, 0.30f}, {0.66f, 0.57f}, {0.74f, 0.86f}, {0.68f, 0.58f},
    {0.75f, 0.87f}, {0.67f, 0.90f}, {0.69f, 0.91f}}};

// side view, sitting back into an invisible chair, arms up along the ears
static const YogaPose CHAIR = {"CHAIR", -1, {
    {0.40f, 0.28f}, {0.44f, 0.36f}, {0.45f, 0.38f}, {0.46f, 0.39f},
    {0.36f, 0.28f}, {0.29f, 0.20f}, {0.37f, 0.29f}, {0.30f, 0.21f},
    {0.58f, 0.62f}, {0.46f, 0.72f}, {0.50f, 0.90f}, {0.47f, 0.73f},
    {0.51f, 0.90f}, {0.44f, 0.92f}, {0.45f, 0.92f}}};

// side view, folded at the hips, hands reaching the floor
static const YogaPose FOLD = {"FORWARD FOLD", -1, {
    {0.42f, 0.82f}, {0.43f, 0.74f}, {0.43f, 0.72f}, {0.44f, 0.73f},
    {0.45f, 0.80f}, {0.44f, 0.89f}, {0.46f, 0.80f}, {0.45f, 0.89f},
    {0.54f, 0.54f}, {0.52f, 0.72f}, {0.52f, 0.90f}, {0.53f, 0.72f},
    {0.53f, 0.90f}, {0.46f, 0.92f}, {0.47f, 0.92f}}};

// front view, legs wide, torso tilted — one hand to the shin, one to the sky
static const YogaPose TRIANGLE = {"TRIANGLE", 0, {
    {0.33f, 0.30f}, {0.38f, 0.36f}, {0.34f, 0.40f}, {0.42f, 0.32f},
    {0.33f, 0.52f}, {0.31f, 0.65f}, {0.46f, 0.22f}, {0.50f, 0.12f},
    {0.50f, 0.55f}, {0.32f, 0.72f}, {0.30f, 0.90f}, {0.68f, 0.72f},
    {0.70f, 0.90f}, {0.26f, 0.92f}, {0.74f, 0.92f}}};

// side view, kneeling folded over the knees, arms stretched out in front
static const YogaPose CHILD = {"CHILDS POSE", -1, {
    {0.33f, 0.82f}, {0.40f, 0.79f}, {0.42f, 0.80f}, {0.43f, 0.81f},
    {0.33f, 0.86f}, {0.24f, 0.88f}, {0.34f, 0.86f}, {0.25f, 0.88f},
    {0.66f, 0.76f}, {0.62f, 0.88f}, {0.76f, 0.90f}, {0.63f, 0.88f},
    {0.77f, 0.90f}, {0.83f, 0.91f}, {0.84f, 0.92f}}};

// side view, lying with the chest lifted — feet pointed, tops on the floor
static const YogaPose COBRA = {"COBRA", -1, {
    {0.29f, 0.60f}, {0.31f, 0.64f}, {0.32f, 0.66f}, {0.34f, 0.67f},
    {0.32f, 0.78f}, {0.29f, 0.90f}, {0.35f, 0.79f}, {0.32f, 0.91f},
    {0.55f, 0.87f}, {0.71f, 0.89f}, {0.87f, 0.89f}, {0.72f, 0.90f},
    {0.88f, 0.90f}, {0.94f, 0.92f}, {0.95f, 0.93f}}};

// side view, one straight line from head to heels on straight arms
static const YogaPose PLANK = {"PLANK", -1, {
    {0.26f, 0.50f}, {0.31f, 0.56f}, {0.32f, 0.57f}, {0.33f, 0.58f},
    {0.31f, 0.73f}, {0.30f, 0.90f}, {0.32f, 0.74f}, {0.31f, 0.90f},
    {0.52f, 0.67f}, {0.64f, 0.74f}, {0.76f, 0.82f}, {0.65f, 0.75f},
    {0.77f, 0.83f}, {0.80f, 0.90f}, {0.81f, 0.91f}}};

// side view, deep lunge with the arms reaching overhead
static const YogaPose WARRIOR1 = {"WARRIOR 1", -1, {
    {0.46f, 0.26f}, {0.48f, 0.34f}, {0.48f, 0.36f}, {0.49f, 0.37f},
    {0.46f, 0.24f}, {0.45f, 0.12f}, {0.48f, 0.25f}, {0.47f, 0.13f},
    {0.50f, 0.60f}, {0.38f, 0.72f}, {0.36f, 0.90f}, {0.63f, 0.78f},
    {0.72f, 0.88f}, {0.30f, 0.92f}, {0.78f, 0.91f}}};

// side view, on the back with the hips lifted, knees bent, feet flat
static const YogaPose BRIDGE = {"BRIDGE", -1, {
    {0.24f, 0.88f}, {0.30f, 0.86f}, {0.32f, 0.87f}, {0.33f, 0.88f},
    {0.40f, 0.89f}, {0.48f, 0.90f}, {0.41f, 0.89f}, {0.49f, 0.90f},
    {0.52f, 0.70f}, {0.64f, 0.66f}, {0.68f, 0.88f}, {0.65f, 0.67f},
    {0.69f, 0.88f}, {0.74f, 0.90f}, {0.75f, 0.91f}}};

// side view, flat on the back, arms at the sides — rest
static const YogaPose SAVASANA = {"SAVASANA", -1, {
    {0.20f, 0.86f}, {0.26f, 0.87f}, {0.28f, 0.87f}, {0.29f, 0.88f},
    {0.38f, 0.89f}, {0.46f, 0.90f}, {0.39f, 0.90f}, {0.47f, 0.91f},
    {0.50f, 0.88f}, {0.64f, 0.88f}, {0.78f, 0.88f}, {0.65f, 0.89f},
    {0.79f, 0.89f}, {0.81f, 0.84f}, {0.82f, 0.85f}}};

// stable voice-clip index for a pose regardless of which flow it's in
// (see docs/AUDIO.md, voice range 0..19)
inline int poseVoice(const YogaPose* p) {
  static const YogaPose* reg[] = {
      &MOUNTAIN, &STAR,   &WARRIOR,  &TRIANGLE, &TREE,     &CHAIR, &FOLD,
      &DOWN_DOG, &COBRA,  &CHILD,    &PLANK,    &WARRIOR1, &BRIDGE, &SAVASANA};
  for (int i = 0; i < int(sizeof(reg) / sizeof(reg[0])); i++)
    if (reg[i] == p) return i;
  return 0;
}

}

class YogaScene : public Scene {
 public:
  explicit YogaScene(Settings& settings) : settings_(settings) {}

  const char* name() const { return "Yoga"; }
  bool autoplayEligible() const { return false; }

  void start(Context& ctx) {
    step_ = 0;
    buildRoutine();
    paused_ = false;
    started_ = false;
    stepStartMs_ = ctx.nowMs;
  }

  bool input(Context& ctx, Key k) {
    if (!started_) {  // setup screen: OK begins, up/down sets the pace
      if (k == Key::Select) {
        started_ = true;
        stepStartMs_ = ctx.nowMs;
        ctx.audio.play(Cue::StartBell);
        ctx.audio.voice(yoga_poses::poseVoice(routine_[step_]));
        return true;
      }
      if (k == Key::Up || k == Key::Down) {
        int s = settings_.yogaHoldSec + ((k == Key::Up) ? 5 : -5);
        settings_.yogaHoldSec = uint16_t(s < 5 ? 5 : (s > 60 ? 60 : s));
        return true;
      }
      return false;
    }
    if (k == Key::Select) {
      paused_ = !paused_;
      return true;
    }
    if (k == Key::Up || k == Key::Down) {  // adjust pace (shared setting)
      int s = settings_.yogaHoldSec + ((k == Key::Up) ? -5 : 5);
      if (s < 5) s = 5;
      if (s > 60) s = 60;
      settings_.yogaHoldSec = uint16_t(s);
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    // reflect a flow change made in settings
    if (settings_.yogaProgram != lastProgram_) buildRoutine();

    if (!started_) {
      char l0[16], l1[16];
      snprintf(l0, sizeof(l0), "%s  %d POSES", flowName(), count_);
      snprintf(l1, sizeof(l1), "HOLD %d SEC", settings_.yogaHoldSec);
      const char* lines[] = {l0, l1};
      intro::draw(ctx, "YOGA", lines, 2, guide::titleColor());
      return 60;
    }

    ctx.fb.clear();

    uint32_t holdMs = uint32_t(settings_.yogaHoldSec) * 1000;
    uint32_t elapsed = ctx.nowMs - stepStartMs_;
    if (elapsed >= holdMs && paused_) elapsed = holdMs - 1;
    if (paused_) {
      stepStartMs_ = ctx.nowMs - elapsed;  // freeze progress
    } else if (elapsed >= holdMs) {
      step_ = (step_ + 1) % count_;
      stepStartMs_ = ctx.nowMs;
      elapsed = 0;
      // pose change: chime, then speak the new pose name
      ctx.audio.play(Cue::Chime);
      ctx.audio.voice(yoga_poses::poseVoice(routine_[step_]));
    }

    const YogaPose& pose = *routine_[step_];
    const YogaPose& prev = *routine_[(step_ + count_ - 1) % count_];

    // tween from the previous pose during the first TRANSITION_MS
    float t = elapsed >= TRANSITION_MS ? 1.0f : elapsed / float(TRANSITION_MS);
    t = t * t * (3.0f - 2.0f * t);  // smoothstep ease

    float j[15][2];
    guide::tweenPose(prev.j, pose.j, t, j);

    guide::drawFigure(ctx, j, pose.face, settings_.yogaBody);

    guide::drawTopBar(ctx, pose.name);

    char counter[8];
    snprintf(counter, sizeof(counter), "%d.%d", step_ + 1, count_);
    font3x5::drawText(ctx.fb, counter, ctx.fb.width() - 16, 2, 1,
                      guide::mutedColor());

    if (paused_) {  // pause icon left of the counter
      ctx.fb.rect(ctx.fb.width() - 26, 2, 2, 5, guide::titleColor());
      ctx.fb.rect(ctx.fb.width() - 22, 2, 2, 5, guide::titleColor());
    }

    // hold-progress bar along the bottom, teal like the mat
    int barW = int(float(ctx.fb.width()) * elapsed / holdMs);
    ctx.fb.hLine(0, barW, ctx.fb.height() - 1, guide::matColor());

    return 33;
  }

 private:
  static const int MAX_STEPS = 8;
  static const uint32_t TRANSITION_MS = 1400;

  Settings& settings_;
  const YogaPose* routine_[MAX_STEPS];
  int count_ = 0;
  int step_ = 0;
  uint8_t lastProgram_ = 255;
  bool paused_;
  bool started_;
  uint32_t stepStartMs_;

  const char* flowName() const {
    static const char* names[3] = {"SUN SALUTE", "STANDING", "WIND DOWN"};
    return names[settings_.yogaProgram > 2 ? 0 : settings_.yogaProgram];
  }

  // three flows that make sense as sequences, not a random pose cycle
  void buildRoutine() {
    using namespace yoga_poses;
    lastProgram_ = settings_.yogaProgram;
    switch (settings_.yogaProgram) {
      case 1:  // standing strength and balance
        routine_[0] = &MOUNTAIN; routine_[1] = &CHAIR;
        routine_[2] = &WARRIOR1; routine_[3] = &WARRIOR;
        routine_[4] = &TRIANGLE; routine_[5] = &STAR;
        routine_[6] = &TREE;
        count_ = 7;
        break;
      case 2:  // wind down to rest
        routine_[0] = &FOLD;   routine_[1] = &DOWN_DOG;
        routine_[2] = &CHILD;  routine_[3] = &COBRA;
        routine_[4] = &BRIDGE; routine_[5] = &SAVASANA;
        count_ = 6;
        break;
      default:  // sun salutation: the classic energizing loop
        routine_[0] = &MOUNTAIN; routine_[1] = &FOLD;
        routine_[2] = &PLANK;    routine_[3] = &COBRA;
        routine_[4] = &DOWN_DOG; routine_[5] = &FOLD;
        count_ = 6;
        break;
    }
    if (step_ >= count_) step_ = 0;
  }
};

}

#endif
