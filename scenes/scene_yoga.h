// Yoga routine player. Poses are skeletons — 13 joints in a normalized
// 0..1 space — so a pose is data, transitions tween smoothly between
// keyframes, and the figure scales to any display size. No artwork needed;
// adding a pose means writing 13 coordinate pairs.
#ifndef CORIOLIS_SCENE_YOGA_H
#define CORIOLIS_SCENE_YOGA_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"

namespace coriolis {

struct YogaPose {
  const char* name;
  // x,y pairs, 0..1: head, neck, lSho, rSho, lElb, lWri, rElb, rWri,
  // pelvis, lKnee, lAnk, rKnee, rAnk
  float j[13][2];
};

namespace yoga_poses {

// front view, standing straight, arms at the sides
static const YogaPose MOUNTAIN = {"MOUNTAIN", {
    {0.50f, 0.14f}, {0.50f, 0.26f}, {0.42f, 0.29f}, {0.58f, 0.29f},
    {0.40f, 0.42f}, {0.39f, 0.54f}, {0.60f, 0.42f}, {0.61f, 0.54f},
    {0.50f, 0.53f}, {0.46f, 0.70f}, {0.45f, 0.90f}, {0.54f, 0.70f},
    {0.55f, 0.90f}}};

// front view, legs wide, arms raised in a V
static const YogaPose STAR = {"STAR", {
    {0.50f, 0.14f}, {0.50f, 0.27f}, {0.42f, 0.30f}, {0.58f, 0.30f},
    {0.30f, 0.21f}, {0.20f, 0.11f}, {0.70f, 0.21f}, {0.80f, 0.11f},
    {0.50f, 0.53f}, {0.38f, 0.70f}, {0.28f, 0.90f}, {0.62f, 0.70f},
    {0.72f, 0.90f}}};

// front view, right foot against the standing leg, palms together overhead
static const YogaPose TREE = {"TREE", {
    {0.50f, 0.12f}, {0.50f, 0.24f}, {0.43f, 0.27f}, {0.57f, 0.27f},
    {0.37f, 0.16f}, {0.48f, 0.05f}, {0.63f, 0.16f}, {0.52f, 0.05f},
    {0.50f, 0.52f}, {0.47f, 0.70f}, {0.46f, 0.90f}, {0.63f, 0.62f},
    {0.51f, 0.68f}}};

// wide lunge, arms straight out over the legs
static const YogaPose WARRIOR = {"WARRIOR 2", {
    {0.50f, 0.18f}, {0.50f, 0.30f}, {0.42f, 0.33f}, {0.58f, 0.33f},
    {0.28f, 0.33f}, {0.13f, 0.33f}, {0.72f, 0.33f}, {0.87f, 0.33f},
    {0.50f, 0.56f}, {0.31f, 0.70f}, {0.28f, 0.90f}, {0.66f, 0.73f},
    {0.75f, 0.90f}}};

// side view, the inverted V: hands planted left, hips high, feet right
static const YogaPose DOWN_DOG = {"DOWN DOG", {
    {0.30f, 0.62f}, {0.36f, 0.52f}, {0.34f, 0.54f}, {0.36f, 0.55f},
    {0.26f, 0.70f}, {0.19f, 0.88f}, {0.28f, 0.71f}, {0.21f, 0.89f},
    {0.56f, 0.30f}, {0.66f, 0.57f}, {0.75f, 0.88f}, {0.68f, 0.58f},
    {0.77f, 0.89f}}};

// side view, lying with the chest lifted on straight arms
static const YogaPose COBRA = {"COBRA", {
    {0.26f, 0.50f}, {0.31f, 0.60f}, {0.32f, 0.62f}, {0.34f, 0.63f},
    {0.32f, 0.74f}, {0.29f, 0.88f}, {0.35f, 0.75f}, {0.32f, 0.89f},
    {0.55f, 0.84f}, {0.71f, 0.86f}, {0.88f, 0.88f}, {0.72f, 0.87f},
    {0.89f, 0.89f}}};

}

class YogaScene : public Scene {
 public:
  const char* name() const { return "Yoga"; }
  bool autoplayEligible() const { return false; }

  void start(Context& ctx) {
    routine_[0] = &yoga_poses::MOUNTAIN;
    routine_[1] = &yoga_poses::STAR;
    routine_[2] = &yoga_poses::WARRIOR;
    routine_[3] = &yoga_poses::TREE;
    routine_[4] = &yoga_poses::DOWN_DOG;
    routine_[5] = &yoga_poses::COBRA;
    step_ = 0;
    paused_ = false;
    stepStartMs_ = ctx.nowMs;
  }

  bool input(Context& ctx, Key k) {
    if (k == Key::Select) {
      paused_ = !paused_;
      return true;
    }
    if (k == Key::Up || k == Key::Down) {  // adjust pace
      holdMs_ += (k == Key::Up) ? -5000 : 5000;
      if (holdMs_ < 5000) holdMs_ = 5000;
      if (holdMs_ > 60000) holdMs_ = 60000;
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    uint32_t elapsed = ctx.nowMs - stepStartMs_;
    if (paused_) {
      stepStartMs_ = ctx.nowMs - elapsed;  // freeze progress
    } else if (elapsed >= holdMs_) {
      step_ = (step_ + 1) % STEPS;
      stepStartMs_ = ctx.nowMs;
      elapsed = 0;
      // pose change: on the device this is where the DFPlayer chime and
      // spoken pose name fire
    }

    const YogaPose& pose = *routine_[step_];
    const YogaPose& prev = *routine_[(step_ + STEPS - 1) % STEPS];

    // tween from the previous pose during the first TRANSITION_MS
    float t = elapsed >= TRANSITION_MS ? 1.0f : elapsed / float(TRANSITION_MS);
    t = t * t * (3.0f - 2.0f * t);  // smoothstep ease

    float j[13][2];
    for (int i = 0; i < 13; i++) {
      j[i][0] = prev.j[i][0] + (pose.j[i][0] - prev.j[i][0]) * t;
      j[i][1] = prev.j[i][1] + (pose.j[i][1] - prev.j[i][1]) * t;
    }

    drawFigure(ctx, j);

    // pose name top center, step counter top right
    RGB textColor = ctx.palette->lookup(48);
    int nameW = font3x5::textWidth(pose.name, 1);
    font3x5::drawText(ctx.fb, pose.name, (ctx.fb.width() - nameW) / 2, 2, 1,
                      textColor);

    char counter[8];
    snprintf(counter, sizeof(counter), "%d.%d", step_ + 1, STEPS);
    font3x5::drawText(ctx.fb, counter, ctx.fb.width() - 16, 2, 1,
                      RGB(90, 90, 90));

    if (paused_) {
      font3x5::drawText(ctx.fb, "PAUSE", 2, 2, 1, RGB(200, 200, 200));
    }

    // hold-progress bar along the bottom, same idiom as the clock
    int barW = int(float(ctx.fb.width()) * elapsed / holdMs_);
    RGB barColor = ctx.palette->lookup(96, 90);
    ctx.fb.hLine(0, barW, ctx.fb.height() - 1, barColor);

    return 33;
  }

 private:
  static const int STEPS = 6;
  static const uint32_t TRANSITION_MS = 1400;

  const YogaPose* routine_[STEPS];
  int step_;
  bool paused_;
  uint32_t stepStartMs_;
  uint32_t holdMs_ = 15000;

  void drawFigure(Context& ctx, float j[13][2]) {
    // fit the unit space into the display with a margin, keeping it square
    // so poses don't stretch on non-square builds
    int size = ctx.fb.width() < ctx.fb.height() ? ctx.fb.width()
                                                : ctx.fb.height();
    int span = size - 14;
    int ox = (ctx.fb.width() - span) / 2;
    int oy = (ctx.fb.height() - span) / 2 + 3;  // leave room for the title

    int px[13], py[13];
    for (int i = 0; i < 13; i++) {
      px[i] = ox + int(j[i][0] * span);
      py[i] = oy + int(j[i][1] * span);
    }

    RGB body = ctx.palette->lookup(0);
    RGB limbs = ctx.palette->lookup(24);

    // torso: shoulders bar, spine, neck
    ctx.fb.thickLine(px[2], py[2], px[3], py[3], body);
    ctx.fb.thickLine(px[1], py[1], px[8], py[8], body);

    // arms: shoulder -> elbow -> wrist
    ctx.fb.thickLine(px[2], py[2], px[4], py[4], limbs);
    ctx.fb.thickLine(px[4], py[4], px[5], py[5], limbs);
    ctx.fb.thickLine(px[3], py[3], px[6], py[6], limbs);
    ctx.fb.thickLine(px[6], py[6], px[7], py[7], limbs);

    // legs: pelvis -> knee -> ankle
    ctx.fb.thickLine(px[8], py[8], px[9], py[9], limbs);
    ctx.fb.thickLine(px[9], py[9], px[10], py[10], limbs);
    ctx.fb.thickLine(px[8], py[8], px[11], py[11], limbs);
    ctx.fb.thickLine(px[11], py[11], px[12], py[12], limbs);

    // head: filled circle joined to the neck
    int headR = span / 16;
    if (headR < 3) headR = 3;
    ctx.fb.thickLine(px[1], py[1], px[0], py[0], body);
    ctx.fb.fillCircle(px[0], py[0], headR, body);
  }
};

}

#endif
