// Digital rain — Matrix-style streams falling down the display, each with a
// bright head and a fading tail. Themed by the current palette, so it reads
// green on Forest, blue on Ocean, and so on.
#ifndef CORIOLIS_SCENE_RAIN_H
#define CORIOLIS_SCENE_RAIN_H

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class RainScene : public Scene {
 public:
  const char* name() const { return "Rain"; }

  void start(Context& ctx) {
    ctx.fb.clear();
    int cols = ctx.fb.width() / CW;
    for (int i = 0; i < cols; i++) {
      dropY_[i] = -float(random8(0, uint8_t(ctx.fb.height())));
      speed_[i] = randSpeed();
    }
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.dimAll(205);  // fade the trailing streams

    const int W = ctx.fb.width(), H = ctx.fb.height();
    int cols = W / CW;

    for (int i = 0; i < cols; i++) {
      dropY_[i] += speed_[i];
      int y = int(dropY_[i]);
      int x = i * CW;

      RGB head = ctx.palette->lookupBright(uint8_t(150 + i * 3));
      RGB neck = head;
      neck.dim(150);
      for (int dx = 0; dx < CW && x + dx < W; dx++) {
        ctx.fb.set(x + dx, y, head);
        ctx.fb.set(x + dx, y - 1, neck);
      }

      if (y > H + int(random8(0, 40))) {  // respawn above, staggered
        dropY_[i] = -float(random8(0, uint8_t(H / 2)));
        speed_[i] = randSpeed();
      }
    }

    return 40;
  }

 private:
  static const int CW = 2;  // stream width in pixels
  float dropY_[128];
  float speed_[128];

  static float randSpeed() { return 0.6f + random8(0, 100) / 100.0f * 1.6f; }
};

}

#endif
