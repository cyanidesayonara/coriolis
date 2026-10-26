// Boot splash: a swirl of particles spirals into the center — deflected
// like air in a Coriolis field — and the wordmark resolves out of it, then
// the shell cross-fades to the clock. About two and a half seconds; any key
// skips it. Deterministic and heap-free like every scene.
#ifndef CORIOLIS_SPLASH_H
#define CORIOLIS_SPLASH_H

#include <math.h>

#include "../core/scene.h"
#include "../core/font5x7.h"
#include "../core/math8.h"

namespace coriolis {

class SplashScene : public Scene {
 public:
  static const uint32_t DURATION_MS = 2600;

  const char* name() const { return "Hello"; }
  bool autoplayEligible() const { return false; }

  void start(Context& ctx) {
    startMs_ = ctx.nowMs;
    ctx.fb.clear();
  }

  bool done(uint32_t nowMs) const { return nowMs - startMs_ >= DURATION_MS; }

  uint32_t draw(Context& ctx) {
    FrameBuffer& fb = ctx.fb;
    const int W = fb.width(), H = fb.height();
    const float cx = W * 0.5f, cy = H * 0.5f - 4;
    float t = (ctx.nowMs - startMs_) / 1000.0f;

    fb.dimAll(185);  // particle trails

    // particles: each starts on a wide ring and spirals inward, arriving
    // between 0.9s and 1.5s; the spin quickens as the radius shrinks
    const float maxR = (W > H ? W : H) * 0.75f;
    for (int i = 0; i < PARTICLES; i++) {
      uint32_t h = hash(uint32_t(i) * 2654435761u + 17u);
      float a0 = (h & 0xFFFF) / 65536.0f * TWO_PI_F;
      float arrive = 0.9f + ((h >> 16) & 0xFF) / 255.0f * 0.6f;
      float p = t / arrive;
      if (p >= 1.0f) continue;
      float e = p * p * (3.0f - 2.0f * p);
      float r = maxR * (1.0f - e) * (0.55f + ((h >> 24) & 0x3F) / 140.0f);
      float a = a0 + e * e * 5.5f;
      int x = int(cx + cosf(a) * r), y = int(cy + sinf(a) * r * 0.92f);
      uint8_t hue = uint8_t(120 + (i * 7) % 80);  // cyan .. violet
      fb.set(x, y, hsv(hue, 190, 255));
    }

    // a ring flashes outward where the swirl collapses
    if (t > 1.0f && t < 1.9f) {
      float k = (t - 1.0f) / 0.9f;
      int rr = int(6 + k * (W * 0.45f));
      RGB ring = hsv(150, 120, 255);
      ring.dim(uint8_t((1.0f - k) * 200));
      fb.circle(int(cx), int(cy), rr, ring);
    }

    // the wordmark fades up over the collapsed swirl
    if (t > 1.0f) {
      float k = (t - 1.0f) / 0.6f;
      if (k > 1.0f) k = 1.0f;
      uint8_t b = uint8_t(k * 255);
      const char* word = "coriolis";
      int scale = font5x7::textWidth(word, 2) <= W - 6 ? 2 : 1;
      int y = int(cy) - font5x7::GLYPH_H * scale / 2 - 1;
      RGB top = hsv(130, 140, 255), bottom = hsv(185, 200, 255);
      top.dim(b);
      bottom.dim(b);
      font5x7::drawCentered(fb, word, W / 2, y, scale, top, bottom);

      if (t > 1.5f && H >= 100) {
        float k2 = (t - 1.5f) / 0.5f;
        if (k2 > 1.0f) k2 = 1.0f;
        uint8_t g = uint8_t(k2 * 120);
        font5x7::drawCentered(fb, "wall clock & more", W / 2,
                              y + font5x7::GLYPH_H * scale + 8, 1,
                              RGB(g, g, uint8_t(g + g / 8)));
      }
    }
    return 16;
  }

 private:
  static const int PARTICLES = 110;
  uint32_t startMs_ = 0;

  static uint32_t hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
  }
};

}

#endif
