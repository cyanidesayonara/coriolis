// Toast — a brief label that slides up at the bottom of the display to
// confirm an action (scene changed, palette cycled, overlay toggled). Fades
// out on its own. Cheap: just a dimmed panel and a line of text.
#ifndef CORIOLIS_TOAST_H
#define CORIOLIS_TOAST_H

#include <string.h>

#include "../core/scene.h"
#include "../core/font.h"

namespace coriolis {

class Toast {
 public:
  void show(const char* msg, uint32_t nowMs, uint32_t durMs = 1300) {
    strncpy(msg_, msg, sizeof(msg_) - 1);
    msg_[sizeof(msg_) - 1] = 0;
    untilMs_ = nowMs + durMs;
  }

  void draw(Context& ctx) {
    if (ctx.nowMs >= untilMs_) return;
    uint32_t left = untilMs_ - ctx.nowMs;

    int w = font3x5::textWidth(msg_, 1);
    int bw = w + 8, bh = 11;
    int ox = (ctx.fb.width() - bw) / 2;
    int oy = ctx.fb.height() - bh - 3;

    for (int y = oy; y < oy + bh; y++)
      for (int x = ox; x < ox + bw; x++)
        if (ctx.fb.contains(x, y)) ctx.fb.at(x, y).dim(38);

    // fade the text over the last 300ms
    uint8_t b = left > 300 ? 255 : uint8_t(left * 255 / 300);
    ctx.fb.rect(ox, oy, bw, 1, RGB(scale8(120, b), scale8(120, b), scale8(120, b)));
    font3x5::drawText(ctx.fb, msg_, ox + 4, oy + 3, 1, RGB(b, b, b));
  }

 private:
  char msg_[24] = {0};
  uint32_t untilMs_ = 0;
};

}

#endif
