// Yoga routine player. Poses are skeletons — 15 joints in a normalized
// 0..1 space — so a pose is data, transitions tween smoothly between
// keyframes, and the figure scales to any display size. Toes are part of
// the pose: down dog holds the feet flexed (heels reaching, toes toward
// the hands) while cobra points them (tops of the feet on the floor).
//
// Five flows. One-sided poses are authored once (right side) and played
// twice — the left side is the mirrored skeleton (guide::mirrorPose) with a
// side tag and a spoken "switch sides". A session runs setup card -> 3-2-1
// -> the flow -> a Namaste card; it ends instead of looping forever.
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
#include "../core/audio.h"

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

// side view, on all fours with the spine rounded up and the head tucked
static const YogaPose CAT = {"CAT", -1, {
    {0.27f, 0.70f}, {0.31f, 0.64f}, {0.33f, 0.59f}, {0.34f, 0.60f},
    {0.32f, 0.75f}, {0.31f, 0.90f}, {0.33f, 0.76f}, {0.32f, 0.90f},
    {0.62f, 0.62f}, {0.62f, 0.89f}, {0.76f, 0.90f}, {0.63f, 0.89f},
    {0.77f, 0.90f}, {0.82f, 0.91f}, {0.83f, 0.92f}}};

// side view, on all fours with the belly dropped and the gaze lifted
static const YogaPose COW = {"COW", -1, {
    {0.25f, 0.51f}, {0.29f, 0.58f}, {0.32f, 0.64f}, {0.33f, 0.65f},
    {0.32f, 0.77f}, {0.31f, 0.90f}, {0.33f, 0.78f}, {0.32f, 0.90f},
    {0.62f, 0.60f}, {0.62f, 0.89f}, {0.76f, 0.90f}, {0.63f, 0.89f},
    {0.77f, 0.90f}, {0.82f, 0.91f}, {0.83f, 0.92f}}};

// side view, front knee over the ankle, back knee down, arms overhead
static const YogaPose LOW_LUNGE = {"LOW LUNGE", -1, {
    {0.41f, 0.30f}, {0.43f, 0.38f}, {0.43f, 0.40f}, {0.44f, 0.41f},
    {0.41f, 0.27f}, {0.40f, 0.15f}, {0.42f, 0.28f}, {0.41f, 0.16f},
    {0.48f, 0.64f}, {0.33f, 0.70f}, {0.33f, 0.90f}, {0.64f, 0.89f},
    {0.78f, 0.90f}, {0.27f, 0.91f}, {0.84f, 0.91f}}};

// side view, front shin folded under, back leg long, tall through the spine
static const YogaPose PIGEON = {"PIGEON", -1, {
    {0.40f, 0.42f}, {0.41f, 0.50f}, {0.41f, 0.52f}, {0.42f, 0.53f},
    {0.39f, 0.68f}, {0.37f, 0.88f}, {0.40f, 0.69f}, {0.38f, 0.88f},
    {0.47f, 0.81f}, {0.33f, 0.87f}, {0.46f, 0.89f}, {0.66f, 0.87f},
    {0.85f, 0.89f}, {0.50f, 0.90f}, {0.90f, 0.91f}}};

// side view, balanced on the sit bones in a V, arms reaching forward
static const YogaPose BOAT = {"BOAT", -1, {
    {0.67f, 0.43f}, {0.65f, 0.51f}, {0.64f, 0.53f}, {0.65f, 0.54f},
    {0.55f, 0.61f}, {0.45f, 0.63f}, {0.56f, 0.62f}, {0.46f, 0.64f},
    {0.56f, 0.86f}, {0.44f, 0.72f}, {0.32f, 0.58f}, {0.45f, 0.73f},
    {0.33f, 0.59f}, {0.29f, 0.54f}, {0.30f, 0.55f}}};

// stable pose id for voice clips regardless of which flow it's in
// (voice::POSE_BASE + id; see docs/AUDIO.md)
inline int poseId(const YogaPose* p) {
  static const YogaPose* reg[] = {
      &MOUNTAIN, &STAR,   &WARRIOR,  &TRIANGLE, &TREE,     &CHAIR, &FOLD,
      &DOWN_DOG, &COBRA,  &CHILD,    &PLANK,    &WARRIOR1, &BRIDGE, &SAVASANA,
      &CAT,      &COW,    &LOW_LUNGE, &PIGEON,  &BOAT};
  for (int i = 0; i < int(sizeof(reg) / sizeof(reg[0])); i++)
    if (reg[i] == p) return i;
  return 0;
}

}

class YogaScene : public Scene {
 public:
  explicit YogaScene(Settings& settings) : settings_(settings) {}

  static const int FLOW_COUNT = 5;

  const char* name() const { return "Yoga"; }
  bool autoplayEligible() const { return false; }

  static const char* flowName(int i) {
    static const char* names[FLOW_COUNT] = {"Morning wake-up", "Sun salutation",
                                            "Standing balance", "Hips & back",
                                            "Bedtime"};
    return names[(i < 0 || i >= FLOW_COUNT) ? 0 : i];
  }

  void start(Context&) {
    phase_ = SETUP;
    buildRoutine();
  }

  bool input(Context& ctx, Key k) {
    switch (phase_) {
      case SETUP:
        if (k == Key::Select) { begin(ctx); return true; }
        if (k == Key::Up || k == Key::Down) {  // pick the flow
          int d = k == Key::Up ? 1 : FLOW_COUNT - 1;
          settings_.yogaProgram = uint8_t((settings_.yogaProgram + d) % FLOW_COUNT);
          buildRoutine();
          return true;
        }
        return false;
      case COUNTDOWN:
        if (k == Key::Select) { startFlow(ctx); return true; }
        if (k == Key::Back) { phase_ = SETUP; return true; }
        return true;  // nothing else should leave mid-countdown
      case FLOW:
        if (k == Key::Select) { paused_ = !paused_; return true; }
        if (k == Key::Up || k == Key::Down) {  // pace (shared setting)
          int s = settings_.yogaHoldSec + ((k == Key::Up) ? 5 : -5);
          settings_.yogaHoldSec = uint16_t(s < 5 ? 5 : (s > 60 ? 60 : s));
          return true;
        }
        if (k == Key::Right) {
          if (step_ + 1 >= count_) finish(ctx);
          else goTo(ctx, step_ + 1);
          return true;
        }
        if (k == Key::Left) { goTo(ctx, step_ > 0 ? step_ - 1 : 0); return true; }
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
    if (settings_.yogaProgram != lastProgram_ && phase_ == SETUP) buildRoutine();

    if (phase_ == SETUP) {
      char l0[24], l1[24];
      snprintf(l0, sizeof(l0), "%s", flowName(settings_.yogaProgram));
      snprintf(l1, sizeof(l1), "%d poses, %ds hold", count_ % 100,
               settings_.yogaHoldSec % 100);
      const char* lines[] = {l0, l1};
      intro::draw(ctx, "Yoga", lines, 2, guide::titleColor());
      return 33;
    }

    if (phase_ == COUNTDOWN) {
      uint32_t e = ctx.nowMs - phaseStartMs_;
      int sec = int(e / 1000);
      // ticks on 2 and 1 only: a tick at 3 would cut "get ready" short
      if (sec != lastTick_ && sec >= 1 && sec < 3) {
        lastTick_ = sec;
        ctx.audio.play(Cue::Tick);
      }
      if (e < COUNTDOWN_MS) {
        guide::drawCountdown(ctx, e, COUNTDOWN_MS, steps_[0].pose->name);
        return 33;
      }
      startFlow(ctx);
    }

    if (phase_ == DONE) {
      char stats[24], dur[8];
      guide::formatDuration(sessionMs_, dur, sizeof(dur));
      snprintf(stats, sizeof(stats), "%d poses  %s", count_ % 100, dur);
      guide::drawCompletion(ctx, "Namaste", stats, ctx.nowMs - phaseStartMs_);
      return 33;
    }

    ctx.fb.clear();

    uint32_t holdMs = uint32_t(settings_.yogaHoldSec) * 1000;
    uint32_t elapsed = ctx.nowMs - stepStartMs_;
    if (elapsed >= holdMs && paused_) elapsed = holdMs - 1;
    if (paused_) {
      stepStartMs_ = ctx.nowMs - elapsed;  // freeze progress
    } else {
      sessionMs_ += ctx.nowMs - lastDrawMs_;
      if (elapsed >= holdMs) {
        lastDrawMs_ = ctx.nowMs;
        if (step_ + 1 >= count_) {
          finish(ctx);
          return 33;
        }
        goTo(ctx, step_ + 1);
        elapsed = 0;
      }
    }
    lastDrawMs_ = ctx.nowMs;

    // cues: halfway through a long hold, then a tick for each of the last
    // three seconds before the change
    int secsLeft = int((holdMs - elapsed + 999) / 1000);
    if (!paused_) {
      if (!halfwayDone_ && holdMs >= 20000 && elapsed >= holdMs / 2) {
        halfwayDone_ = true;
        ctx.audio.play(Cue::Halfway);
      }
      if (secsLeft <= 3 && secsLeft != lastTick_ && step_ + 1 < count_) {
        lastTick_ = secsLeft;
        ctx.audio.play(Cue::Tick);
      }
    }

    // tween from the previous step during the first TRANSITION_MS
    float t = elapsed >= TRANSITION_MS ? 1.0f : elapsed / float(TRANSITION_MS);
    t = t * t * (3.0f - 2.0f * t);  // smoothstep ease

    float from[15][2], to[15][2], j[15][2];
    int face = stepJoints(step_, to);
    stepJoints(fromStep_, from);
    guide::tweenPose(from, to, t, j);

    // once settled in the pose, breathe: the chest and head lift a hair on
    // the inhale (~4s cycle) and the shoulders open, so a held pose isn't a
    // frozen mannequin. Skipped mid-transition and while paused.
    if (t >= 1.0f && !paused_) {
      float breath = sinf(ctx.nowMs * 0.0016f);
      const float a = 0.006f;
      j[0][1] -= breath * a;          // head
      j[1][1] -= breath * a * 0.7f;   // neck
      j[2][1] -= breath * a * 0.5f;   // shoulders lift
      j[3][1] -= breath * a * 0.5f;
      j[2][0] -= breath * a * 0.4f;   // and open
      j[3][0] += breath * a * 0.4f;
    }

    guide::drawFigure(ctx, j, face, settings_.yogaBody);

    const Step& st = steps_[step_];
    guide::drawTopBar(ctx, st.pose->name);
    guide::drawSideTag(ctx, st.side);

    char counter[8];
    snprintf(counter, sizeof(counter), "%d.%d", (step_ + 1) % 100, count_ % 100);
    font3x5::drawText(ctx.fb, counter,
                      ctx.fb.width() - font3x5::textWidth(counter, 1) - 2, 2, 1,
                      guide::mutedColor());

    // what's next, for the last few seconds of the hold
    if (step_ + 1 < count_ && secsLeft <= 4 && holdMs > 6000) {
      const Step& nx = steps_[step_ + 1];
      bool otherSide = nx.pose == st.pose;
      guide::drawNextBadge(ctx, otherSide ? "OTHER SIDE" : nx.pose->name,
                           otherSide ? 0 : nx.side, secsLeft);
    }

    if (paused_) guide::drawPaused(ctx);

    // hold-progress bar along the bottom, teal like the mat
    int barW = int(float(ctx.fb.width()) * elapsed / holdMs);
    ctx.fb.hLine(0, barW, ctx.fb.height() - 1, guide::matColor());

    return 33;
  }

 private:
  static const int MAX_STEPS = 16;
  static const uint32_t TRANSITION_MS = 1400;
  static const uint32_t COUNTDOWN_MS = 3000;

  enum Phase { SETUP, COUNTDOWN, FLOW, DONE };
  struct Step {
    const YogaPose* pose;
    int8_t side;  // 0 symmetric, +1 right (as authored), -1 left (mirrored)
  };

  Settings& settings_;
  Step steps_[MAX_STEPS];
  int count_ = 0;
  int step_ = 0;
  int fromStep_ = 0;
  uint8_t lastProgram_ = 255;
  Phase phase_ = SETUP;
  bool paused_ = false;
  bool halfwayDone_ = false;
  int lastTick_ = -1;
  uint32_t stepStartMs_ = 0;
  uint32_t phaseStartMs_ = 0;
  uint32_t lastDrawMs_ = 0;
  uint32_t sessionMs_ = 0;

  // joints for a step, mirrored for the left side; returns the facing
  int stepJoints(int i, float out[15][2]) const {
    const Step& st = steps_[i < 0 ? 0 : (i >= count_ ? count_ - 1 : i)];
    if (st.side < 0) {
      guide::mirrorPose(st.pose->j, out);
      return -st.pose->face;
    }
    for (int k = 0; k < 15; k++) {
      out[k][0] = st.pose->j[k][0];
      out[k][1] = st.pose->j[k][1];
    }
    return st.pose->face;
  }

  void begin(Context& ctx) {
    buildRoutine();
    phase_ = COUNTDOWN;
    phaseStartMs_ = ctx.nowMs;
    lastTick_ = -1;
    ctx.audio.voice(voice::GET_READY);
  }

  void startFlow(Context& ctx) {
    phase_ = FLOW;
    paused_ = false;
    sessionMs_ = 0;
    lastDrawMs_ = ctx.nowMs;
    step_ = 0;
    fromStep_ = 0;
    stepStartMs_ = ctx.nowMs;
    halfwayDone_ = false;
    lastTick_ = -1;
    ctx.audio.play(Cue::StartBell);
    ctx.audio.voice(voice::POSE_BASE + yoga_poses::poseId(steps_[0].pose));
  }

  void goTo(Context& ctx, int to) {
    if (to >= count_) to = count_ - 1;
    fromStep_ = step_;
    step_ = to;
    stepStartMs_ = ctx.nowMs;
    halfwayDone_ = false;
    lastTick_ = -1;
    // pose change: chime, then the spoken pose (or "switch sides")
    ctx.audio.play(Cue::Chime);
    bool otherSide = fromStep_ != step_ &&
                     steps_[fromStep_].pose == steps_[step_].pose;
    ctx.audio.voice(otherSide ? voice::SWITCH_SIDES
                              : voice::POSE_BASE +
                                    yoga_poses::poseId(steps_[step_].pose));
  }

  void finish(Context& ctx) {
    phase_ = DONE;
    phaseStartMs_ = ctx.nowMs;
    ctx.audio.play(Cue::FinishBell);
    ctx.audio.voice(voice::NAMASTE);
  }

  void add(const YogaPose& p, bool bilateral) {
    if (count_ + (bilateral ? 2 : 1) > MAX_STEPS) return;
    steps_[count_].pose = &p;
    steps_[count_++].side = int8_t(bilateral ? 1 : 0);
    if (bilateral) {
      steps_[count_].pose = &p;
      steps_[count_++].side = -1;
    }
  }

  // five flows that make sense as sequences, not a random pose cycle
  void buildRoutine() {
    using namespace yoga_poses;
    lastProgram_ = settings_.yogaProgram;
    count_ = 0;
    switch (settings_.yogaProgram) {
      case 0:  // morning wake-up: spine and hamstrings
        add(CHILD, false); add(COW, false); add(CAT, false);
        add(DOWN_DOG, false); add(LOW_LUNGE, true); add(FOLD, false);
        add(MOUNTAIN, false);
        break;
      case 2:  // standing strength and balance
        add(CHAIR, false); add(WARRIOR1, true); add(WARRIOR, true);
        add(TRIANGLE, true); add(STAR, false); add(TREE, true);
        break;
      case 3:  // hips and back, for desk days
        add(CHILD, false); add(CAT, false); add(COW, false);
        add(DOWN_DOG, false); add(LOW_LUNGE, true); add(PIGEON, true);
        add(BRIDGE, false); add(SAVASANA, false);
        break;
      case 4:  // bedtime: slow floor holds
        add(CHILD, false); add(CAT, false); add(COW, false);
        add(PIGEON, true); add(BRIDGE, false); add(BOAT, false);
        add(SAVASANA, false);
        break;
      default:  // sun salutation: the classic energizing sequence
        add(MOUNTAIN, false); add(FOLD, false); add(PLANK, false);
        add(COBRA, false); add(DOWN_DOG, false); add(WARRIOR1, true);
        add(FOLD, false); add(MOUNTAIN, false);
        break;
    }
    if (step_ >= count_) step_ = 0;
  }
};

}

#endif
