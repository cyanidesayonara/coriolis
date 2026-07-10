// Tiny bitmap font rendered as plain pixels into the framebuffer, with an
// integer scale factor for big chunky clock digits. Backend-independent —
// this is what Borealis never had in its simulator.
#ifndef CORIOLIS_FONT_H
#define CORIOLIS_FONT_H

#include "display.h"

namespace coriolis {

// 3x5 glyphs, one byte per row, 3 low bits used (MSB = left pixel).
// Coverage: digits, colon, minus, degree. Extend as scenes need more.
namespace font3x5 {

static const int GLYPH_W = 3;
static const int GLYPH_H = 5;

inline const uint8_t* glyph(char c) {
  static const uint8_t digits[10][5] = {
      {0b111, 0b101, 0b101, 0b101, 0b111},  // 0
      {0b010, 0b110, 0b010, 0b010, 0b111},  // 1
      {0b111, 0b001, 0b111, 0b100, 0b111},  // 2
      {0b111, 0b001, 0b111, 0b001, 0b111},  // 3
      {0b101, 0b101, 0b111, 0b001, 0b001},  // 4
      {0b111, 0b100, 0b111, 0b001, 0b111},  // 5
      {0b111, 0b100, 0b111, 0b101, 0b111},  // 6
      {0b111, 0b001, 0b001, 0b010, 0b010},  // 7
      {0b111, 0b101, 0b111, 0b101, 0b111},  // 8
      {0b111, 0b101, 0b111, 0b001, 0b111},  // 9
  };
  static const uint8_t colon[5] = {0b000, 0b010, 0b000, 0b010, 0b000};
  static const uint8_t minus[5] = {0b000, 0b000, 0b111, 0b000, 0b000};
  static const uint8_t degree[5] = {0b010, 0b101, 0b010, 0b000, 0b000};
  static const uint8_t blank[5] = {0, 0, 0, 0, 0};

  if (c >= '0' && c <= '9') return digits[c - '0'];
  if (c == ':') return colon;
  if (c == '-') return minus;
  if (c == '*') return degree;  // '*' stands in for the degree sign
  return blank;
}

// draw one glyph at pixel scale; returns the advance in pixels
inline int drawChar(FrameBuffer& fb, char c, int x, int y, int scale,
                    const RGB& color) {
  const uint8_t* rows = glyph(c);
  for (int gy = 0; gy < GLYPH_H; gy++) {
    for (int gx = 0; gx < GLYPH_W; gx++) {
      if (rows[gy] & (0b100 >> gx)) {
        fb.rect(x + gx * scale, y + gy * scale, scale, scale, color);
      }
    }
  }
  return (GLYPH_W + 1) * scale;  // one glyph-column of spacing
}

inline int textWidth(const char* s, int scale) {
  int n = 0;
  while (s[n]) n++;
  if (n == 0) return 0;
  return n * (GLYPH_W + 1) * scale - scale;  // no trailing space
}

inline void drawText(FrameBuffer& fb, const char* s, int x, int y, int scale,
                     const RGB& color) {
  for (int i = 0; s[i]; i++) {
    x += drawChar(fb, s[i], x, y, scale, color);
  }
}

}

}

#endif
