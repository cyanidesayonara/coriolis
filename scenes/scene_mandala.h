// Mandala — the jazzed-up Spiro. The same nested oscillators, but every
// point is mirrored 8 ways into a kaleidoscope, the orbit radius breathes,
// the colors are brighter and additive (so overlaps bloom), and the hue
// races. Where Spiro is a quiet line drawing, this is a living mandala.
#ifndef CORIOLIS_SCENE_MANDALA_H
#define CORIOLIS_SCENE_MANDALA_H

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class MandalaScene : public Scene {
 public:
  const char* name() const { return "Mandala"; }

  void start(Context& ctx) {
    ctx.fb.clear();
    theta1_ = theta2_ = hue_ = pulse_ = 0;
    count_ = 5;
    lastT1_ = lastT2_ = lastHue_ = lastPulse_ = ctx.nowMs;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.dimAll(243);  // slow fade -> long blooming trails

    const int W = ctx.fb.width(), H = ctx.fb.height();
    const int cx = W / 2, cy = H / 2;

    // the orbit radius breathes in and out
    int r = W / 5 + (sin8(pulse_) * (W / 11)) / 255;
    uint8_t offset = uint8_t(256 / count_);

    for (int i = 0; i < count_; i++) {
      uint8_t a = uint8_t(theta1_ + i * offset);
      int ox = (int(sin8(a)) - 128) * r / 128;
      int oy = (int(cos8(a)) - 128) * r / 128;

      uint8_t b = uint8_t(theta2_ + i * offset);
      int px = ox + (int(sin8(b)) - 128) * r / 128;
      int py = oy + (int(cos8(b)) - 128) * r / 128;

      RGB c = ctx.palette->lookupBright(uint8_t(hue_ + i * offset), 200);

      // 8-fold kaleidoscope: the point, its axis mirrors, and the diagonal
      // swap and its mirrors
      ctx.fb.at(cx + px, cy + py).add(c);
      ctx.fb.at(cx - px, cy + py).add(c);
      ctx.fb.at(cx + px, cy - py).add(c);
      ctx.fb.at(cx - px, cy - py).add(c);
      ctx.fb.at(cx + py, cy + px).add(c);
      ctx.fb.at(cx - py, cy + px).add(c);
      ctx.fb.at(cx + py, cy - px).add(c);
      ctx.fb.at(cx - py, cy - px).add(c);
    }

    // all time-based, so it looks the same at any frame rate
    if (ctx.nowMs - lastT2_ >= 12) { lastT2_ = ctx.nowMs; theta2_ += 2; }
    if (ctx.nowMs - lastT1_ >= 14) { lastT1_ = ctx.nowMs; theta1_ += 1; }
    if (ctx.nowMs - lastHue_ >= 20) { lastHue_ = ctx.nowMs; hue_ += 2; }
    if (ctx.nowMs - lastPulse_ >= 30) {
      lastPulse_ = ctx.nowMs;
      pulse_ += 1;
      // drift the arm count on the slow pulse wrap, for variety
      if (pulse_ == 0) {
        count_++;
        if (count_ > 8) count_ = 3;
      }
    }

    return 8;
  }

 private:
  uint8_t theta1_, theta2_, hue_, pulse_;
  int count_;
  uint32_t lastT1_, lastT2_, lastHue_, lastPulse_;
};

}

#endif
