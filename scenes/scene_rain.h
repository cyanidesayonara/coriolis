// Digital rain — Matrix-style columns of glyphs falling down the display,
// each stream with a bright white leading character and a tail of glyphs
// fading into the palette color. Pair it with the MATRIX palette for the
// authentic black-and-green look.
#ifndef CORIOLIS_SCENE_RAIN_H
#define CORIOLIS_SCENE_RAIN_H

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"

namespace coriolis {

class RainScene : public Scene {
 public:
  const char* name() const { return "Rain"; }

  void start(Context& ctx) {
    int cols = ctx.fb.width() / CW;
    int rows = ctx.fb.height() / CH;
    for (int c = 0; c < cols; c++) {
      head_[c] = -float(random8(0, uint8_t(rows + 4)));
      speed_[c] = randSpeed();
      for (int r = 0; r < rows && r < MAXR; r++) grid_[c][r] = randChar();
    }
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    const int W = ctx.fb.width(), H = ctx.fb.height();
    int cols = W / CW, rows = H / CH;
    if (rows > MAXR) rows = MAXR;

    RGB white(210, 255, 210);  // the leading glyph

    for (int c = 0; c < cols; c++) {
      head_[c] += speed_[c];
      int headRow = int(head_[c]);

      for (int k = 0; k < TRAIL; k++) {
        int r = headRow - k;
        if (r < 0 || r >= rows) continue;
        RGB color;
        if (k == 0) {
          color = white;
        } else {
          color = ctx.palette->lookupBright(150);
          color.dim(uint8_t(200 - k * (190 / TRAIL)));
        }
        font3x5::drawChar(ctx.fb, grid_[c][r], c * CW, r * CH, 1, color);
      }

      // flicker: occasionally swap a glyph somewhere in the visible trail
      if (random8() < 26) {
        int r = headRow - random8(0, TRAIL);
        if (r >= 0 && r < rows) grid_[c][r] = randChar();
      }

      // respawn above once the whole tail has cleared the bottom
      if (headRow - TRAIL > rows) {
        head_[c] = -float(random8(0, uint8_t(rows / 2 + 1)));
        speed_[c] = randSpeed();
      }
    }

    return 50;
  }

 private:
  static const int CW = 4;    // cell width  (3px glyph + gap)
  static const int CH = 6;    // cell height (5px glyph + gap)
  static const int TRAIL = 13;
  static const int MAXC = 48;
  static const int MAXR = 48;

  float head_[MAXC];
  float speed_[MAXC];
  char grid_[MAXC][MAXR];

  static float randSpeed() { return 0.14f + random8(0, 100) / 100.0f * 0.34f; }

  static char randChar() {
    uint8_t v = random8(0, 36);
    return v < 10 ? char('0' + v) : char('A' + (v - 10));
  }
};

}

#endif
