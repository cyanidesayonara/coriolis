// Shared intro/setup card for guides and games: a title, a few setup lines
// (adjustable before you begin), and a gently pulsing start prompt. Scenes
// hold a `started_` flag, show this until OK is pressed, then run.
#ifndef CORIOLIS_INTRO_H
#define CORIOLIS_INTRO_H

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/font5x7.h"
#include "../core/math8.h"

namespace coriolis {
namespace intro {

inline void draw(Context& ctx, const char* title, const char* const* lines,
                 int lineCount, RGB accent) {
  ctx.fb.clear();
  const int W = ctx.fb.width(), H = ctx.fb.height();

  // title in the big rounded face, with a soft vertical gradient
  int scale = (font5x7::textWidth(title, 2) <= W - 8 && H >= 100) ? 2 : 1;
  int ty = H / 2 - 16 - lineCount * 5;
  if (ty < 5) ty = 5;
  RGB deep = accent;
  deep.dim(150);
  font5x7::drawCentered(ctx.fb, title, W / 2, ty, scale, accent, deep);

  // a short accent rule under the title
  int ruleY = ty + font5x7::GLYPH_H * scale + 4;
  RGB rule = accent;
  rule.dim(90);
  ctx.fb.hLine(W / 2 - 10, W / 2 + 10, ruleY, rule);

  int y = ruleY + 6;
  for (int i = 0; i < lineCount; i++) {
    font5x7::drawCentered(ctx.fb, lines[i], W / 2, y, 1, RGB(170, 170, 178));
    y += 11;
  }

  // up/down hint, then a pulsing start pill
  if (H >= 100) {  // the widescreen build has no room for it
    const char* hint = "UP/DOWN TO ADJUST";
    int hw = font3x5::textWidth(hint, 1);
    font3x5::drawText(ctx.fb, hint, (W - hw) / 2, H - 27, 1, RGB(80, 80, 88));
  }

  const char* p = "OK to start";
  int pw = font5x7::textWidth(p, 1);
  int bw = pw + 14, bh = 13;
  int bx = (W - bw) / 2, by = H - 18;
  uint8_t pulse = beatsin8(24, ctx.nowMs, 60, 120);
  RGB fill = accent;
  fill.dim(pulse);
  ctx.fb.rect(bx + 1, by, bw - 2, bh, fill);
  ctx.fb.rect(bx, by + 1, 1, bh - 2, fill);
  ctx.fb.rect(bx + bw - 1, by + 1, 1, bh - 2, fill);
  font5x7::drawText(ctx.fb, p, bx + 7, by + 3, 1, RGB(255, 255, 255));
}

}
}

#endif
