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

// side view, lying with the chest lifted — feet pointed, tops on the floor
static const YogaPose COBRA = {"COBRA", -1, {
    {0.29f, 0.60f}, {0.31f, 0.64f}, {0.32f, 0.66f}, {0.34f, 0.67f},
    {0.32f, 0.78f}, {0.29f, 0.90f}, {0.35f, 0.79f}, {0.32f, 0.91f},
    {0.55f, 0.87f}, {0.71f, 0.89f}, {0.87f, 0.89f}, {0.72f, 0.90f},
    {0.88f, 0.90f}, {0.94f, 0.92f}, {0.95f, 0.93f}}};

}

class YogaScene : public Scene {
 public:
  explicit YogaScene(Settings& settings) : settings_(settings) {}

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
    ctx.fb.clear();

    uint32_t holdMs = uint32_t(settings_.yogaHoldSec) * 1000;
    uint32_t elapsed = ctx.nowMs - stepStartMs_;
    if (elapsed >= holdMs && paused_) elapsed = holdMs - 1;
    if (paused_) {
      stepStartMs_ = ctx.nowMs - elapsed;  // freeze progress
    } else if (elapsed >= holdMs) {
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

    float j[15][2];
    for (int i = 0; i < 15; i++) {
      j[i][0] = prev.j[i][0] + (pose.j[i][0] - prev.j[i][0]) * t;
      j[i][1] = prev.j[i][1] + (pose.j[i][1] - prev.j[i][1]) * t;
    }

    drawFigure(ctx, j, pose.face);

    guide::drawTopBar(ctx, pose.name);

    char counter[8];
    snprintf(counter, sizeof(counter), "%d.%d", step_ + 1, STEPS);
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
  static const int STEPS = 6;
  static const uint32_t TRANSITION_MS = 1400;

  Settings& settings_;
  const YogaPose* routine_[STEPS];
  int step_;
  bool paused_;
  uint32_t stepStartMs_;

  void drawFigure(Context& ctx, float j[15][2], int face) {
    // fit the unit space into the display with a margin, keeping it square
    // so poses don't stretch on non-square builds
    int size = ctx.fb.width() < ctx.fb.height() ? ctx.fb.width()
                                                : ctx.fb.height();
    int span = size - 14;
    int ox = (ctx.fb.width() - span) / 2;
    int oy = (ctx.fb.height() - span) / 2 + 3;  // leave room for the top bar

    int px[15], py[15];
    for (int i = 0; i < 15; i++) {
      px[i] = ox + int(j[i][0] * span);
      py[i] = oy + int(j[i][1] * span);
    }

    RGB body = guide::bodyColor();
    RGB limbs = guide::limbColor();

    // the mat grounds every pose
    int matY = oy + int(0.93f * span);
    ctx.fb.rect(ox + span / 10, matY, span - span / 5, 2, guide::matColor());

    // torso: a quad with real width in every view. In side poses the two
    // shoulders nearly coincide, so the width comes from the perpendicular
    // of the shoulders->pelvis axis with enforced minimums — the trunk
    // never collapses to a sliver.
    {
      float smx = (px[2] + px[3]) * 0.5f, smy = (py[2] + py[3]) * 0.5f;
      float axx = px[8] - smx, axy = py[8] - smy;
      float alen = sqrtf(axx * axx + axy * axy);
      if (alen < 1.0f) alen = 1.0f;
      float perpX = -axy / alen, perpY = axx / alen;

      // reach the true shoulder joints so the arms root on the torso; the
      // clamp only catches degenerate data
      float dxs = float(px[2] - px[3]), dys = float(py[2] - py[3]);
      float shoHalf = 0.5f * sqrtf(dxs * dxs + dys * dys);
      float minSho = span * 0.050f, maxSho = span * 0.095f;
      if (shoHalf < minSho) shoHalf = minSho;
      if (shoHalf > maxSho) shoHalf = maxSho;
      float hipHalf = span * (settings_.yogaBody == 0 ? 0.048f : 0.038f);

      int sx0 = int(smx + perpX * shoHalf), sy0 = int(smy + perpY * shoHalf);
      int sx1 = int(smx - perpX * shoHalf), sy1 = int(smy - perpY * shoHalf);
      int hx0 = int(px[8] + perpX * hipHalf), hy0 = int(py[8] + perpY * hipHalf);
      int hx1 = int(px[8] - perpX * hipHalf), hy1 = int(py[8] - perpY * hipHalf);

      ctx.fb.fillTriangle(sx0, sy0, sx1, sy1, hx0, hy0, body);
      ctx.fb.fillTriangle(sx1, sy1, hx1, hy1, hx0, hy0, body);

      // rounded shoulders and hips, and the neck runs from the head down
      // to the torso's real top edge — no gap
      ctx.fb.fillCircle(int(smx), int(smy), 2, body);
      ctx.fb.fillCircle(px[8], py[8], int(hipHalf), body);
      ctx.fb.boldLine(int(smx), int(smy), px[0], py[0], body);
    }

    // arms: shoulder -> elbow -> wrist, deltoid roots, elbow caps, fists
    ctx.fb.thickLine(px[2], py[2], px[4], py[4], limbs);
    ctx.fb.thickLine(px[4], py[4], px[5], py[5], limbs);
    ctx.fb.thickLine(px[3], py[3], px[6], py[6], limbs);
    ctx.fb.thickLine(px[6], py[6], px[7], py[7], limbs);
    ctx.fb.fillCircle(px[2], py[2], 2, limbs);
    ctx.fb.fillCircle(px[3], py[3], 2, limbs);
    ctx.fb.fillCircle(px[4], py[4], 1, limbs);
    ctx.fb.fillCircle(px[6], py[6], 1, limbs);
    ctx.fb.fillCircle(px[5], py[5], 2, limbs);
    ctx.fb.fillCircle(px[7], py[7], 2, limbs);

    // legs: beefier than arms — bold lines with knee caps; feet run
    // ankle -> toe, so flexed vs pointed is decided by the pose data
    ctx.fb.boldLine(px[8], py[8], px[9], py[9], limbs);
    ctx.fb.boldLine(px[9], py[9], px[10], py[10], limbs);
    ctx.fb.boldLine(px[8], py[8], px[11], py[11], limbs);
    ctx.fb.boldLine(px[11], py[11], px[12], py[12], limbs);
    ctx.fb.fillCircle(px[9], py[9], 2, limbs);
    ctx.fb.fillCircle(px[11], py[11], 2, limbs);
    ctx.fb.thickLine(px[10], py[10], px[13], py[13], limbs);
    ctx.fb.thickLine(px[12], py[12], px[14], py[14], limbs);

    // head last, over the neck
    int headR = span / 16;
    if (headR < 3) headR = 3;
    ctx.fb.fillCircle(px[0], py[0], headR, body);

    // hair: a bun, top-back of the head relative to where she's facing
    if (settings_.yogaBody == 0) {
      int bunX = face == 0 ? px[0] : px[0] - face * (headR - 1);
      int bunY = face == 0 ? py[0] - headR : py[0] - headR + 2;
      ctx.fb.fillCircle(bunX, bunY, 2, guide::limbColor());
    }

    // face: two eyes head-on, one in profile
    RGB eyeColor(70, 30, 30);
    if (face == 0) {
      ctx.fb.set(px[0] - 2, py[0] - 1, eyeColor);
      ctx.fb.set(px[0] + 2, py[0] - 1, eyeColor);
    } else {
      ctx.fb.set(px[0] + face * 3, py[0] - 1, eyeColor);
      ctx.fb.set(px[0] + face * (headR - 1), py[0] + 1, eyeColor);  // nose tip
    }
  }
};

}

#endif
