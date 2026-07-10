// The classic heat-diffusion fire — one of the patterns Borealis had to
// disable to fit in 64KB. Its heat buffer (one byte per pixel) is a rounding
// error on modern targets.
#ifndef CORIOLIS_SCENE_FIRE_H
#define CORIOLIS_SCENE_FIRE_H

#include <string.h>

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class FireScene : public Scene {
 public:
  const char* name() const { return "Fire"; }

  void start(Context&) { memset(heat_, 0, sizeof(heat_)); }

  uint32_t draw(Context& ctx) {
    const int w = ctx.fb.width();
    const int h = ctx.fb.height();

    for (int x = 0; x < w; x++) {
      // cool every cell a little
      for (int y = 0; y < h; y++) {
        heat_[idx(x, y)] =
            qsub8(heat_[idx(x, y)], random8(0, uint8_t(COOLING * 10 / h + 2)));
      }

      // heat drifts upward, diffusing
      for (int y = 0; y < h - 2; y++) {
        heat_[idx(x, y)] = uint8_t((heat_[idx(x, y + 1)] +
                                    heat_[idx(x, y + 2)] * 2) / 3);
      }

      // random sparks at the bottom
      if (random8() < SPARKING) {
        int y = h - 1 - randomInt(3);
        heat_[idx(x, y)] = qadd8(heat_[idx(x, y)], random8(160, 255));
      }
    }

    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        // fire always uses the heat ramp regardless of selected palette
        ctx.fb.set(x, y, palettes::byIndex(1).lookup(
                             scale8(heat_[idx(x, y)], 240)));
      }
    }

    return 30;
  }

 private:
  // the two tuning dials: more COOLING = shorter flames, more SPARKING =
  // busier fire. These aim for flames over ~2/3 of the height with dark
  // sky above, rather than a full-screen inferno.
  static const uint8_t COOLING = 100;
  static const uint8_t SPARKING = 90;

  uint8_t heat_[PIXELS];

  static int idx(int x, int y) { return y * WIDTH + x; }
};

}

#endif
