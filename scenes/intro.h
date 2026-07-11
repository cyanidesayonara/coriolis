// Shared intro/setup card for guides and games: a title, a few setup lines
// (adjustable before you begin), and a blinking start prompt. Scenes hold a
// `started_` flag, show this until OK is pressed, then run.
#ifndef CORIOLIS_INTRO_H
#define CORIOLIS_INTRO_H

#include "../core/scene.h"
#include "../core/font.h"

namespace coriolis {
namespace intro {

inline void draw(Context& ctx, const char* title, const char* const* lines,
                 int lineCount, RGB accent) {
  ctx.fb.clear();

  int tw = font3x5::textWidth(title, 2);
  int ty = ctx.fb.height() / 2 - 8 - lineCount * 5;
  if (ty < 6) ty = 6;
  font3x5::drawText(ctx.fb, title, (ctx.fb.width() - tw) / 2, ty, 2, accent);

  int y = ty + 18;
  for (int i = 0; i < lineCount; i++) {
    int w = font3x5::textWidth(lines[i], 1);
    font3x5::drawText(ctx.fb, lines[i], (ctx.fb.width() - w) / 2, y, 1,
                      RGB(150, 150, 150));
    y += 9;
  }

  // up/down hint, then a blinking start prompt
  const char* hint = "UP DOWN TO SET";
  int hw = font3x5::textWidth(hint, 1);
  font3x5::drawText(ctx.fb, hint, (ctx.fb.width() - hw) / 2,
                    ctx.fb.height() - 24, 1, RGB(80, 80, 80));

  if ((ctx.nowMs / 500) % 2 == 0) {
    const char* p = "OK TO START";
    int pw = font3x5::textWidth(p, 1);
    font3x5::drawText(ctx.fb, p, (ctx.fb.width() - pw) / 2,
                      ctx.fb.height() - 13, 1, accent);
  }
}

}
}

#endif
