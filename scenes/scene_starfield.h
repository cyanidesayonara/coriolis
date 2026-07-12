// Starfield — flying through space at warp. Stars stream out from the center,
// accelerating and brightening as they near the edge, leaving short streaks.
// Themed: white on most palettes, green on Matrix, and so on.
#ifndef CORIOLIS_SCENE_STARFIELD_H
#define CORIOLIS_SCENE_STARFIELD_H

#include <math.h>

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class StarfieldScene : public Scene {
 public:
  const char* name() const { return "Starfield"; }

  void start(Context& ctx) {
    for (int i = 0; i < N; i++) spawn(stars_[i]);
    hue_ = 150;
    lastHue_ = ctx.nowMs;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();
    const int W = ctx.fb.width(), H = ctx.fb.height();
    const int cx = W / 2, cy = H / 2;
    const float maxD = float(cx < cy ? cy : cx);

    for (int i = 0; i < N; i++) {
      Star& s = stars_[i];
      float prev = s.dist;
      s.dist += s.speed * (s.dist * 0.06f + 0.5f);  // accelerate outward

      int x = cx + int(s.dx * s.dist), y = cy + int(s.dy * s.dist);
      if (x < 0 || x >= W || y < 0 || y >= H) { spawn(s); continue; }

      int px = cx + int(s.dx * prev), py = cy + int(s.dy * prev);
      uint8_t b = uint8_t(40 + 215 * (s.dist / maxD > 1 ? 1 : s.dist / maxD));
      RGB c = ctx.palette->lookupBright(hue_);
      c.dim(b);
      ctx.fb.line(px, py, x, y, c);  // the streak
    }

    if (ctx.nowMs - lastHue_ >= 120) { lastHue_ = ctx.nowMs; hue_ += 1; }
    return 30;
  }

 private:
  static const int N = 110;
  struct Star { float dx, dy, dist, speed; };

  Star stars_[N];
  uint8_t hue_;
  uint32_t lastHue_ = 0;

  static void spawn(Star& s) {
    uint8_t a = random8();
    float dx = (int(sin8(a)) - 128) / 128.0f;
    float dy = (int(cos8(a)) - 128) / 128.0f;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.05f) len = 0.05f;
    s.dx = dx / len;
    s.dy = dy / len;
    s.dist = 2.0f + random8(0, 60) / 10.0f;
    s.speed = 0.6f + random8(0, 100) / 80.0f;
  }
};

}

#endif
