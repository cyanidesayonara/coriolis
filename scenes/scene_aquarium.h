// Aquarium — a calm fish tank. Fish drift back and forth at their own pace,
// bubbles rise, seaweed sways, and the water darkens with depth. A fixed
// underwater look, not themed (like the fireplace).
#ifndef CORIOLIS_SCENE_AQUARIUM_H
#define CORIOLIS_SCENE_AQUARIUM_H

#include <math.h>

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class AquariumScene : public Scene {
 public:
  const char* name() const { return "Aquarium"; }

  void start(Context& ctx) {
    const int W = ctx.fb.width(), H = ctx.fb.height();
    for (int i = 0; i < NFISH; i++) {
      Fish& f = fish_[i];
      f.x = float(random8(0, uint8_t(W)));
      f.y = 12 + random8(0, uint8_t(H - 30));
      f.dir = random8() < 128 ? 1 : -1;
      f.speed = 0.2f + random8(0, 100) / 200.0f;
      f.r = 2 + random8(0, 3);
      f.color = fishColors()[i % NCOLOR];
      f.bob = random8();
    }
    for (int i = 0; i < NBUB; i++) resetBubble(bubbles_[i], W, H, true);
    phase_ = 0;
  }

  uint32_t draw(Context& ctx) {
    const int W = ctx.fb.width(), H = ctx.fb.height();
    phase_++;

    // water: darker toward the bottom
    for (int y = 0; y < H; y++) {
      uint8_t b = uint8_t(30 - 24 * y / H);
      ctx.fb.rect(0, y, W, 1, RGB(0, b / 2, b));
    }

    // seaweed swaying up from the sand
    int sandY = H - 5;
    for (int s = 0; s < NWEED; s++) {
      int base = (s * 2 + 1) * W / (NWEED * 2);
      for (int h = 0; h < 16; h++) {
        int y = sandY - h;
        int sway = int(sinf(h * 0.4f + phase_ * 0.05f + s) * (h / 6.0f));
        ctx.fb.set(base + sway, y, RGB(20, 120 - h * 3, 40));
      }
    }

    // bubbles rise
    for (int i = 0; i < NBUB; i++) {
      Bubble& b = bubbles_[i];
      b.y -= b.speed;
      b.x += sinf(b.y * 0.2f) * 0.3f;
      if (b.y < 2) resetBubble(b, W, H, false);
      ctx.fb.set(int(b.x), int(b.y), RGB(120, 190, 220));
    }

    // fish
    for (int i = 0; i < NFISH; i++) {
      Fish& f = fish_[i];
      f.x += f.speed * f.dir;
      if (f.x < -8) f.x = W + 6;
      if (f.x > W + 8) f.x = -6;
      int fy = f.y + int(sinf(phase_ * 0.04f + f.bob) * 2);
      drawFish(ctx, int(f.x), fy, f.r, f.dir, f.color);
    }

    // sand
    ctx.fb.rect(0, sandY, W, H - sandY, RGB(90, 80, 55));

    return 40;
  }

 private:
  static const int NFISH = 7, NBUB = 12, NWEED = 4, NCOLOR = 5;
  struct Fish { float x, speed; int y, dir, r; RGB color; uint8_t bob; };
  struct Bubble { float x, y, speed; };

  Fish fish_[NFISH];
  Bubble bubbles_[NBUB];
  uint32_t phase_;

  static const RGB* fishColors() {
    static const RGB c[NCOLOR] = {{255, 140, 0},  {255, 210, 40}, {230, 60, 60},
                                  {255, 120, 160}, {90, 200, 220}};
    return c;
  }

  static void resetBubble(Bubble& b, int W, int H, bool anywhere) {
    b.x = float(random8(0, uint8_t(W)));
    b.y = anywhere ? float(random8(0, uint8_t(H))) : float(H - 6);
    b.speed = 0.3f + random8(0, 100) / 150.0f;
  }

  void drawFish(Context& ctx, int fx, int fy, int r, int dir, const RGB& c) {
    ctx.fb.fillCircle(fx, fy, r, c);
    int tx = fx - dir * (r + 1);
    ctx.fb.fillTriangle(tx, fy, tx - dir * r, fy - r, tx - dir * r, fy + r, c);
    ctx.fb.set(fx + dir * (r - 1), fy - 1, RGB(15, 15, 15));  // eye
  }
};

}

#endif
