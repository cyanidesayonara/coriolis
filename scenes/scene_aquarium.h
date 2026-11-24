// Aquarium — a calm fish tank. Fish drift back and forth at their own pace,
// tail fins gently wagging, bubbles rise, kelp sways, and the water darkens
// with depth. A fixed underwater look, not themed (like the fireplace).
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
      f.y = 12 + random8(0, uint8_t(H > 32 ? H - 30 : 12));
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

    // water: richer deep teal-blue gradient darkening toward the bottom
    for (int y = 0; y < H; y++) {
      uint8_t b = uint8_t(36 - 28 * y / H);
      ctx.fb.rect(0, y, W, 1, RGB(0, uint8_t(b * 2 / 3), uint8_t(b + 4)));
    }

    // multi-blade kelp fronds swaying up from the sand
    int sandY = H - 6;
    for (int s = 0; s < NWEED; s++) {
      int base = (s * 2 + 1) * W / (NWEED * 2);
      int maxH = H >= 100 ? 22 : 12;
      for (int h = 0; h < maxH; h++) {
        int y = sandY - h;
        float sway = sinf(h * 0.35f + phase_ * 0.045f + s * 1.3f) * (h / 5.5f);
        uint8_t g = uint8_t(135 - h * 3);
        ctx.fb.set(base + int(sway), y, RGB(18, g, 42));
        if (h < maxH - 5) {
          ctx.fb.set(base + 2 + int(sway * 0.85f), y,
                     RGB(12, uint8_t(g * 3 / 4), 32));
        }
      }
    }

    // rising bubbles with soft anti-aliased shimmer
    for (int i = 0; i < NBUB; i++) {
      Bubble& b = bubbles_[i];
      b.y -= b.speed;
      b.x += sinf(b.y * 0.2f) * 0.25f;
      if (b.y < 2) resetBubble(b, W, H, false);
      if (b.big) {
        ctx.fb.fillCircleAA(b.x, b.y, 1.25f, RGB(90, 165, 200));
        ctx.fb.set(int(b.x), int(b.y) - 1, RGB(195, 238, 255));
      } else {
        ctx.fb.set(int(b.x), int(b.y), RGB(130, 200, 228));
      }
    }

    // fish
    for (int i = 0; i < NFISH; i++) {
      Fish& f = fish_[i];
      f.x += f.speed * f.dir;
      if (f.x < -10) f.x = float(W + 8);
      if (f.x > W + 10) f.x = -8.0f;
      float fy = f.y + sinf(phase_ * 0.04f + f.bob) * 2.0f;
      drawFish(ctx, f.x, fy, f.r, f.dir, f.color, f.bob);
    }

    // layered sand bed with small river pebbles
    ctx.fb.hLine(0, W - 1, sandY, RGB(104, 92, 62));
    ctx.fb.rect(0, sandY + 1, W, H - sandY - 1, RGB(82, 72, 48));
    for (int p = 6; p < W - 4; p += 17) {
      int py = sandY + 2 + ((p * 7) % 3);
      ctx.fb.hLine(p, p + 2, py, RGB(64, 56, 40));
    }

    return 40;
  }

 private:
  static const int NFISH = 7, NBUB = 14, NWEED = 5, NCOLOR = 5;
  struct Fish { float x, speed; int y, dir, r; RGB color; uint8_t bob; };
  struct Bubble { float x, y, speed; bool big; };

  Fish fish_[NFISH];
  Bubble bubbles_[NBUB];
  uint32_t phase_;

  static const RGB* fishColors() {
    static const RGB c[NCOLOR] = {{255, 140, 12}, {255, 212, 48}, {235, 68, 68},
                                  {255, 125, 168}, {88, 205, 225}};
    return c;
  }

  static void resetBubble(Bubble& b, int W, int H, bool anywhere) {
    b.x = float(random8(0, uint8_t(W)));
    b.y = anywhere ? float(random8(0, uint8_t(H))) : float(H - 7);
    b.speed = 0.28f + random8(0, 100) / 150.0f;
    b.big = random8() < 75;
  }

  void drawFish(Context& ctx, float fx, float fy, int r, int dir, const RGB& c,
                uint8_t bob) {
    float rx = r * 1.40f;
    float ry = r * 0.90f;
    RGB hi = lerpColor(c, RGB(255, 255, 255), 65);
    RGB fin = c;
    fin.dim(185);

    // wagging tail fin
    float wag = sinf(phase_ * 0.22f + bob) * (r * 0.35f);
    int tx = int(fx - dir * rx);
    int ty = int(fy);
    int tailLen = r + 1;
    ctx.fb.fillTriangle(tx, ty,
                        tx - dir * tailLen, int(fy - r * 0.85f + wag),
                        tx - dir * tailLen, int(fy + r * 0.85f + wag), fin);

    // small dorsal fin
    ctx.fb.fillTriangle(int(fx + dir * 1), int(fy - ry + 0.5f),
                        int(fx - dir * (r * 0.8f)), int(fy - ry - 1.8f),
                        int(fx - dir * (r * 0.4f)), int(fy - ry + 0.5f), fin);

    // shaded oval body
    int x0 = int(floorf(fx - rx - 1)), x1 = int(ceilf(fx + rx + 1));
    int y0 = int(floorf(fy - ry - 1)), y1 = int(ceilf(fy + ry + 1));
    for (int y = y0; y <= y1; y++) {
      float dy = (y + 0.5f - fy) / ry;
      for (int x = x0; x <= x1; x++) {
        float dx = (x + 0.5f - fx) / rx;
        float e2 = dx * dx + dy * dy;
        if (e2 > 1.35f) continue;
        float cov = (1.0f - sqrtf(e2)) * ry + 0.5f;
        if (cov <= 0.0f) continue;
        if (cov > 1.0f) cov = 1.0f;
        RGB col = dy < -0.15f ? lerpColor(c, hi, uint8_t((-dy) * 180)) : c;
        if (cov >= 0.99f) ctx.fb.set(x, y, col);
        else ctx.fb.blend(x, y, col, uint8_t(cov * 255.0f));
      }
    }

    // crisp white eye with dark pupil
    int ex = int(fx + dir * (rx * 0.48f));
    int ey = int(fy - ry * 0.22f);
    ctx.fb.set(ex, ey, RGB(240, 246, 250));
    ctx.fb.set(ex + dir, ey, RGB(15, 20, 28));
  }
};

}

#endif
