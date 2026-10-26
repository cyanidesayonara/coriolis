// The UI font: 5x7 proportional glyphs with real lowercase (descenders use
// an eighth row), tabular digits so clocks don't jitter, and a "smooth"
// scaler that rounds outer corners and fills inner diagonals when a glyph is
// blown up — big clock digits read as rounded LCD numerals instead of a
// staircase of squares. The tiny 3x5 font (font.h) stays for dense HUDs.
#ifndef CORIOLIS_FONT5X7_H
#define CORIOLIS_FONT5X7_H

#include "display.h"

namespace coriolis {
namespace font5x7 {

static const int GLYPH_H = 7;   // cap height; descenders add one row below
static const int ROWS = 8;
static const int DIGIT_W = 5;   // tabular digit cell

// one byte per row, 5 low bits used (bit 4 = leftmost pixel)
inline const uint8_t* glyph(char c) {
  static const uint8_t digits[10][ROWS] = {
      {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0},  // 0
      {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E, 0},  // 1
      {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F, 0},  // 2
      {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E, 0},  // 3
      {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02, 0},  // 4
      {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E, 0},  // 5
      {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E, 0},  // 6
      {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08, 0},  // 7
      {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E, 0},  // 8
      {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C, 0},  // 9
  };
  static const uint8_t upper[26][ROWS] = {
      {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0},  // A
      {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E, 0},  // B
      {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E, 0},  // C
      {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E, 0},  // D
      {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F, 0},  // E
      {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10, 0},  // F
      {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F, 0},  // G
      {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0},  // H
      {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E, 0},  // I
      {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C, 0},  // J
      {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11, 0},  // K
      {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, 0},  // L
      {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11, 0},  // M
      {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0},  // N
      {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0},  // O
      {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10, 0},  // P
      {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D, 0},  // Q
      {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11, 0},  // R
      {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E, 0},  // S
      {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0},  // T
      {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0},  // U
      {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04, 0},  // V
      {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A, 0},  // W
      {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11, 0},  // X
      {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0},  // Y
      {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F, 0},  // Z
  };
  static const uint8_t lower[26][ROWS] = {
      {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0x00},  // a
      {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1E, 0x00},  // b
      {0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E, 0x00},  // c
      {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F, 0x00},  // d
      {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0x00},  // e
      {0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08, 0x00},  // f
      {0x00, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x0E},  // g
      {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11, 0x00},  // h
      {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E, 0x00},  // i
      {0x02, 0x00, 0x06, 0x02, 0x02, 0x02, 0x12, 0x0C},  // j
      {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12, 0x00},  // k
      {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00},  // l
      {0x00, 0x00, 0x1A, 0x15, 0x15, 0x15, 0x11, 0x00},  // m
      {0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11, 0x00},  // n
      {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00},  // o
      {0x00, 0x00, 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10},  // p
      {0x00, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x01},  // q
      {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10, 0x00},  // r
      {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E, 0x00},  // s
      {0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06, 0x00},  // t
      {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D, 0x00},  // u
      {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04, 0x00},  // v
      {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A, 0x00},  // w
      {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00},  // x
      {0x00, 0x00, 0x11, 0x11, 0x11, 0x0F, 0x01, 0x0E},  // y
      {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F, 0x00},  // z
  };
  static const uint8_t colon[ROWS] = {0, 0x04, 0x04, 0, 0x04, 0x04, 0, 0};
  static const uint8_t dot[ROWS] = {0, 0, 0, 0, 0, 0x04, 0x04, 0};
  static const uint8_t comma[ROWS] = {0, 0, 0, 0, 0, 0x04, 0x04, 0x08};
  static const uint8_t minus[ROWS] = {0, 0, 0, 0x0E, 0, 0, 0, 0};
  static const uint8_t plus[ROWS] = {0, 0x04, 0x04, 0x1F, 0x04, 0x04, 0, 0};
  static const uint8_t slash[ROWS] = {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10, 0};
  static const uint8_t amp[ROWS] = {0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D, 0};
  static const uint8_t bang[ROWS] = {0x04, 0x04, 0x04, 0x04, 0x04, 0, 0x04, 0};
  static const uint8_t query[ROWS] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0, 0x04, 0};
  static const uint8_t apos[ROWS] = {0x04, 0x04, 0x08, 0, 0, 0, 0, 0};
  static const uint8_t lparen[ROWS] = {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02, 0};
  static const uint8_t rparen[ROWS] = {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08, 0};
  static const uint8_t degree[ROWS] = {0x0C, 0x12, 0x12, 0x0C, 0, 0, 0, 0};
  static const uint8_t percent[ROWS] = {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03, 0};
  static const uint8_t lt[ROWS] = {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02, 0};
  static const uint8_t gt[ROWS] = {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0};
  static const uint8_t blank[ROWS] = {0, 0, 0, 0, 0, 0, 0, 0};

  if (c >= '0' && c <= '9') return digits[c - '0'];
  if (c >= 'A' && c <= 'Z') return upper[c - 'A'];
  if (c >= 'a' && c <= 'z') return lower[c - 'a'];
  switch (c) {
    case ':': return colon;
    case '.': return dot;
    case ',': return comma;
    case '-': return minus;
    case '+': return plus;
    case '/': return slash;
    case '&': return amp;
    case '!': return bang;
    case '?': return query;
    case '\'': return apos;
    case '(': return lparen;
    case ')': return rparen;
    case '*': return degree;  // '*' stands in for the degree sign
    case '%': return percent;
    case '<': return lt;
    case '>': return gt;
    default: return blank;
  }
}

// columns a glyph actually uses, so text sets proportionally; digits keep
// a fixed cell and space a fixed gap
inline void span(char c, int& first, int& width) {
  if (c == ' ') { first = 0; width = 2; return; }
  if (c >= '0' && c <= '9') { first = 0; width = DIGIT_W; return; }
  const uint8_t* g = glyph(c);
  uint8_t bits = 0;
  for (int r = 0; r < ROWS; r++) bits |= g[r];
  if (!bits) { first = 0; width = 2; return; }
  int lo = 4, hi = 0;
  for (int col = 0; col < 5; col++) {
    if (bits & (0x10 >> col)) {
      if (col < lo) lo = col;
      if (col > hi) hi = col;
    }
  }
  first = lo;
  width = hi - lo + 1;
}

inline int charAdvance(char c) {
  int f, w;
  span(c, f, w);
  return w + 1;
}

inline int textWidth(const char* s, int scale) {
  int w = 0;
  for (int i = 0; s[i]; i++) w += charAdvance(s[i]);
  return w > 0 ? (w - 1) * scale : 0;  // no trailing gap
}

// pixel (gx, gy) of a glyph, with everything outside its cell off
inline bool on(const uint8_t* g, int gx, int gy) {
  if (gx < 0 || gx > 4 || gy < 0 || gy >= ROWS) return false;
  return (g[gy] & (0x10 >> gx)) != 0;
}

// draw one scaled cell. Smooth mode trims a convex corner where both
// neighbours on that side are empty, and fills the inner corner of an empty
// cell flanked by two lit neighbours — so diagonals join up and blocks
// round off. r is the size of the trimmed/filled triangle.
inline void drawCell(FrameBuffer& fb, const uint8_t* g, int gx, int gy, int x,
                     int y, int scale, const RGB& top, const RGB& bottom,
                     int totalH, int rowOffset, bool smooth) {
  bool self = on(g, gx, gy);
  bool L = on(g, gx - 1, gy), R = on(g, gx + 1, gy);
  bool U = on(g, gx, gy - 1), D = on(g, gx, gy + 1);
  int r = smooth ? scale / 2 : 0;
  for (int dy = 0; dy < scale; dy++) {
    int py = y + dy;
    // vertical gradient across the whole line of text
    int ry = rowOffset + dy;
    uint8_t frac = totalH > 1 ? uint8_t(ry * 255 / (totalH - 1)) : 0;
    RGB c = (top == bottom) ? top : lerpColor(top, bottom, frac);
    for (int dx = 0; dx < scale; dx++) {
      int ix = scale - 1 - dx, iy = scale - 1 - dy;
      bool lit;
      if (self) {
        lit = true;
        if (r) {
          if (!L && !U && !on(g, gx - 1, gy - 1) && dx + dy < r) lit = false;
          if (!R && !U && !on(g, gx + 1, gy - 1) && ix + dy < r) lit = false;
          if (!L && !D && !on(g, gx - 1, gy + 1) && dx + iy < r) lit = false;
          if (!R && !D && !on(g, gx + 1, gy + 1) && ix + iy < r) lit = false;
        }
      } else {
        lit = false;
        if (r) {
          if (L && U && dx + dy < r) lit = true;
          if (R && U && ix + dy < r) lit = true;
          if (L && D && dx + iy < r) lit = true;
          if (R && D && ix + iy < r) lit = true;
        }
      }
      if (lit) fb.set(x + dx, py, c);
    }
  }
}

// one glyph; returns the advance in pixels
inline int drawChar(FrameBuffer& fb, char c, int x, int y, int scale,
                    const RGB& top, const RGB& bottom, bool smooth = true) {
  const uint8_t* g = glyph(c);
  int first, width;
  span(c, first, width);
  int ox = x;
  if (c >= '0' && c <= '9') first = 0;  // tabular: draw the full cell
  bool sm = smooth && scale >= 2;
  int totalH = GLYPH_H * scale;
  for (int gy = 0; gy < ROWS; gy++) {
    for (int gx = first - (sm ? 1 : 0); gx < first + width + (sm ? 1 : 0);
         gx++) {
      if (!sm && !on(g, gx, gy)) continue;
      drawCell(fb, g, gx, gy, ox + (gx - first) * scale, y + gy * scale,
               scale, top, bottom, totalH, gy * scale, sm);
    }
  }
  return (width + 1) * scale;
}

inline int drawChar(FrameBuffer& fb, char c, int x, int y, int scale,
                    const RGB& color) {
  return drawChar(fb, c, x, y, scale, color, color);
}

inline void drawText(FrameBuffer& fb, const char* s, int x, int y, int scale,
                     const RGB& top, const RGB& bottom, bool smooth = true) {
  for (int i = 0; s[i]; i++)
    x += drawChar(fb, s[i], x, y, scale, top, bottom, smooth);
}

inline void drawText(FrameBuffer& fb, const char* s, int x, int y, int scale,
                     const RGB& color) {
  drawText(fb, s, x, y, scale, color, color);
}

// centered on column cx
inline void drawCentered(FrameBuffer& fb, const char* s, int cx, int y,
                         int scale, const RGB& top, const RGB& bottom) {
  drawText(fb, s, cx - textWidth(s, scale) / 2, y, scale, top, bottom);
}

inline void drawCentered(FrameBuffer& fb, const char* s, int cx, int y,
                         int scale, const RGB& color) {
  drawCentered(fb, s, cx, y, scale, color, color);
}

}
}

#endif
