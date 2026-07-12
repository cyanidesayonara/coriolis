// Aurora — the northern lights, closing the loop on Aurora -> Borealis ->
// Coriolis. Curtains of light hang from an undulating ridge, shimmering with
// vertical rays and drifting color. Themed: green on Forest/Matrix, blue on
// Ocean, and so on. Additive, so overlapping folds bloom.
#ifndef CORIOLIS_SCENE_AURORA_H
#define CORIOLIS_SCENE_AURORA_H

#include <math.h>

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class AuroraScene : public Scene {
 public:
  const char* name() const { return "Aurora"; }

  void start(Context& ctx) { phase_ = 0; }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();
    const int W = ctx.fb.width(), H = ctx.fb.height();
    const float t = phase_ * 0.018f;

    // two layered curtains — a bright foreground and a fainter, slower one
    for (int layer = 0; layer < 2; layer++) {
      float lt = t * (layer == 0 ? 1.0f : 0.6f) + layer * 2.1f;
      float ridgeBase = H * (layer == 0 ? 0.34f : 0.28f);
      float bright = layer == 0 ? 1.0f : 0.55f;
      uint8_t hueBase = uint8_t(layer == 0 ? 110 : 150);

      for (int x = 0; x < W; x++) {
        // the glowing ridge weaves across the sky
        float w = sinf(x * 0.05f + lt) * 0.5f + sinf(x * 0.11f - lt * 0.7f) * 0.3f +
                  sinf(x * 0.021f + lt * 0.4f) * 0.2f;
        float ridge = ridgeBase + w * H * 0.13f;

        // per-column intensity, with fine vertical ray striations
        float col = 0.5f + 0.5f * sinf(x * 0.08f + lt * 1.2f);
        float ray = 0.65f + 0.35f * sinf(x * 0.5f + lt * 2.3f);
        float inten = col * ray * bright;

        for (int y = 0; y < H; y++) {
          float d = (y < ridge) ? (ridge - y) / (H * 0.22f)   // quick fade up
                                : (y - ridge) / (H * 0.62f);  // curtains hang
          float glow = 1.0f - d;
          if (glow <= 0.0f) continue;
          float b = glow * glow * inten;
          if (b < 0.06f) continue;

          uint8_t hue = uint8_t(hueBase + int((y - ridge) * 1.4f) + int(x * 0.2f));
          uint8_t bb = b > 1.0f ? 255 : uint8_t(b * 255);
          ctx.fb.at(x, y).add(ctx.palette->lookup(hue, bb));
        }
      }
    }

    phase_++;
    return 33;
  }

 private:
  uint32_t phase_;
};

}

#endif
