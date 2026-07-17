// Coriolis — the namesake scene. Particles stream outward inside a rotating
// reference frame, and the Coriolis acceleration (a = -2 Omega x v) bends
// every straight path into a curving arc — the same deflection that spins
// cyclones and curls the aurora's parent winds. Faint spokes rotate at the
// frame's speed; trails make the deflection visible.
#ifndef CORIOLIS_SCENE_CORIOLIS_H
#define CORIOLIS_SCENE_CORIOLIS_H

#include <math.h>

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class CoriolisScene : public Scene {
 public:
  const char* name() const { return "Coriolis"; }

  void start(Context& ctx) {
    ctx.fb.clear();
    for (int i = 0; i < N; i++) launch(p_[i], ctx.fb.width(), true);
    spoke_ = 0.0f;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.dimAll(238);  // trails show the curvature

    const int W = ctx.fb.width(), H = ctx.fb.height();
    const int cx = W / 2, cy = H / 2;
    const float maxR = float(cx < cy ? cx : cy);

    // the rotating frame, hinted by three slow dim spokes
    spoke_ += OMEGA;
    for (int s = 0; s < 3; s++) {
      float a = spoke_ + s * (TWO_PI_F / 3.0f);
      int ex = cx + int(cosf(a) * (maxR - 2));
      int ey = cy + int(sinf(a) * (maxR - 2));
      RGB spokeColor = ctx.palette->lookup(160, 26);
      ctx.fb.line(cx, cy, ex, ey, spokeColor);
    }

    for (int i = 0; i < N; i++) {
      P& q = p_[i];

      // Coriolis deflection: perpendicular to the velocity, strength 2*Omega
      float ax = 2.0f * OMEGA * q.vy;
      float ay = -2.0f * OMEGA * q.vx;
      q.vx += ax;
      q.vy += ay;
      q.x += q.vx;
      q.y += q.vy;
      q.age++;

      float dx = q.x - cx, dy = q.y - cy;
      if (dx * dx + dy * dy > maxR * maxR || q.age > 400) {
        launch(q, W, false);
        continue;
      }

      RGB c = ctx.palette->lookupBright(uint8_t(q.hue + q.age / 3));
      c.dim(uint8_t(160 + (q.age < 48 ? 2 * q.age : 95)));
      ctx.fb.at(int(q.x), int(q.y)).add(c);
    }

    // the eye of the storm
    ctx.fb.fillCircle(cx, cy, 1, ctx.palette->lookupBright(0, 140));

    return 25;
  }

 private:
  static const int N = 16;
  // frame rotation per tick; the sign makes paths curve to the right,
  // northern-hemisphere style
  static constexpr float OMEGA = 0.02f;

  struct P {
    float x, y, vx, vy;
    int age;
    uint8_t hue;
  };
  P p_[N];
  float spoke_;

  void launch(P& q, int size, bool scatter) {
    float cx = size / 2.0f;
    uint8_t a = random8();
    float dirx = (int(sin8(a)) - 128) / 128.0f;
    float diry = (int(cos8(a)) - 128) / 128.0f;
    float len = sqrtf(dirx * dirx + diry * diry);
    if (len < 0.05f) len = 0.05f;
    float speed = 0.55f + random8(0, 100) / 160.0f;
    q.x = cx + dirx * (scatter ? random8(0, uint8_t(size / 3)) : 2.0f);
    q.y = cx + diry * (scatter ? random8(0, uint8_t(size / 3)) : 2.0f);
    q.vx = dirx / len * speed;
    q.vy = diry / len * speed;
    q.age = 0;
    q.hue = random8();
  }
};

}

#endif
