// Toast — a brief label that slides up at the bottom of the display to
// confirm an action (scene changed, palette cycled, overlay toggled). Eases
// in, holds, fades out on its own. Cheap: a darkened rounded pill and one
// line of the UI font.
#ifndef CORIOLIS_TOAST_H
#define CORIOLIS_TOAST_H

#include <string.h>

#include "../core/scene.h"
#include "../core/font5x7.h"

namespace coriolis {

class Toast {
 public:
  void show(const char* msg, uint32_t nowMs, uint32_t durMs = 1400) {
    strncpy(msg_, msg, sizeof(msg_) - 1);
    msg_[sizeof(msg_) - 1] = 0;
    startMs_ = nowMs;
    untilMs_ = nowMs + durMs;
  }

  bool visible(uint32_t nowMs) const { return int32_t(untilMs_ - nowMs) > 0; }

  void draw(Context& ctx) {
    if (!visible(ctx.nowMs)) return;
    uint32_t age = ctx.nowMs - startMs_;
    uint32_t left = untilMs_ - ctx.nowMs;

    int w = font5x7::textWidth(msg_, 1);
    int bw = w + 12, bh = 13;
    int ox = (ctx.fb.width() - bw) / 2;
    int restY = ctx.fb.height() - bh - 4;

    // ease up from below the edge over the first 180ms
    int oy = restY;
    if (age < 180) {
      float t = age / 180.0f;
      t = 1.0f - (1.0f - t) * (1.0f - t);
      oy = ctx.fb.height() + int((restY - ctx.fb.height()) * t);
    }

    // fade over the last 300ms
    uint8_t b = left > 300 ? 255 : uint8_t(left * 255 / 300);

    for (int y = oy; y < oy + bh; y++) {
      for (int x = ox; x < ox + bw; x++) {
        bool corner = (y == oy || y == oy + bh - 1) && (x == ox || x == ox + bw - 1);
        if (corner || !ctx.fb.contains(x, y)) continue;
        ctx.fb.at(x, y).dim(uint8_t(255 - scale8(215, b)));
        if (y == oy) ctx.fb.at(x, y).add(RGB(scale8(70, b), scale8(70, b), scale8(78, b)));
      }
    }
    font5x7::drawText(ctx.fb, msg_, ox + 6, oy + 3, 1, RGB(b, b, b));
  }

 private:
  char msg_[24] = {0};
  uint32_t startMs_ = 0;
  uint32_t untilMs_ = 0;
};

}

#endif
