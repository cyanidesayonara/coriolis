// Lava — metaballs, the lava-lamp look. A handful of blobs drift and bounce
// around the panel; each pixel sums an inverse-square field from every blob,
// and the total is mapped straight through the theme palette. Where fields
// overlap they fuse into soft, stretching shapes. Heat reads as molten lava,
// Ocean as slow bioluminescence, Rainbow as a psychedelic lamp. Pure ambient:
// no fixed colors, it lives entirely off whatever palette is selected.
#ifndef CORIOLIS_SCENE_LAVA_H
#define CORIOLIS_SCENE_LAVA_H

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class LavaScene : public Scene {
 public:
  const char* name() const { return "Lava"; }

  void start(Context& ctx) {
    const int W = ctx.fb.width(), H = ctx.fb.height();
    for (int i = 0; i < NB; i++) {
      b_[i].x = float(random8(20, uint8_t(W - 20)));
      b_[i].y = float(random8(20, uint8_t(H - 20)));
      b_[i].vx = vel();
      b_[i].vy = vel();
      b_[i].r = float(random8(10, 18));
    }
  }

  uint32_t draw(Context& ctx) {
    const int W = ctx.fb.width(), H = ctx.fb.height();

    for (int i = 0; i < NB; i++) {
      Blob& b = b_[i];
      b.x += b.vx;
      b.y += b.vy;
      float m = b.r * 0.3f;
      if (b.x < m) { b.x = m; b.vx = -b.vx; }
      if (b.x > W - m) { b.x = W - m; b.vx = -b.vx; }
      if (b.y < m) { b.y = m; b.vy = -b.vy; }
      if (b.y > H - m) { b.y = H - m; b.vy = -b.vy; }
    }

    for (int y = 0; y < H; y++) {
      for (int x = 0; x < W; x++) {
        float field = 0.0f;
        for (int i = 0; i < NB; i++) {
          float dx = x - b_[i].x, dy = y - b_[i].y;
          field += (b_[i].r * b_[i].r) / (dx * dx + dy * dy + 1.0f);
        }
        if (field < 0.45f) { ctx.fb.set(x, y, RGB(0, 0, 0)); continue; }
        // hold the hottest white for the very cores; spend most of the range
        // on the red-orange body of each blob
        uint8_t idx = field >= 2.6f ? 255
                                    : uint8_t((field - 0.45f) * (255.0f / 2.15f));
        ctx.fb.set(x, y, ctx.palette->lookup(idx));
      }
    }
    return 40;
  }

 private:
  static const int NB = 6;
  struct Blob { float x, y, vx, vy, r; };
  Blob b_[NB];

  // a drift speed that's never quite zero, so no blob stalls in a corner
  static float vel() {
    float v = (float(random8()) / 128.0f - 1.0f) * 0.55f;  // ~ -0.55..0.55
    if (v > -0.15f && v < 0.15f) v = v < 0 ? -0.2f : 0.2f;
    return v;
  }
};

}

#endif
