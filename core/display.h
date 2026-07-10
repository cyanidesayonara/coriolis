// The framebuffer every scene draws into. Backends (desktop simulator,
// Teensy/SmartMatrix) copy it to their real output once per frame — scenes
// never talk to hardware.
#ifndef CORIOLIS_DISPLAY_H
#define CORIOLIS_DISPLAY_H

#include <string.h>

#include "config.h"
#include "color.h"

namespace coriolis {

class FrameBuffer {
 public:
  RGB px[PIXELS];

  int width() const { return WIDTH; }
  int height() const { return HEIGHT; }

  bool contains(int x, int y) const {
    return x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT;
  }

  // out-of-bounds writes go to a scratch pixel instead of memory beyond the
  // buffer, so scenes can draw carelessly near edges
  RGB& at(int x, int y) {
    if (!contains(x, y)) return scratch_;
    return px[y * WIDTH + x];
  }

  const RGB& at(int x, int y) const {
    if (!contains(x, y)) return scratchConst_;
    return px[y * WIDTH + x];
  }

  void set(int x, int y, const RGB& c) { at(x, y) = c; }

  void clear() { memset(px, 0, sizeof(px)); }

  void fill(const RGB& c) {
    for (int i = 0; i < PIXELS; i++) px[i] = c;
  }

  // scale every pixel toward black; the classic trail/fade primitive
  void dimAll(uint8_t keep) {
    for (int i = 0; i < PIXELS; i++) px[i].dim(keep);
  }

  void hLine(int x0, int x1, int y, const RGB& c) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    for (int x = x0; x <= x1; x++) set(x, y, c);
  }

  void vLine(int x, int y0, int y1, const RGB& c) {
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    for (int y = y0; y <= y1; y++) set(x, y, c);
  }

  void rect(int x, int y, int w, int h, const RGB& c) {
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++) set(xx, yy, c);
  }

  void line(int x0, int y0, int x1, int y1, const RGB& c) {
    int dx = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
    int dy = y1 - y0 >= 0 ? y0 - y1 : y1 - y0;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
      set(x0, y0, c);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }

  // a line with body — reads far better than a 1px line at chunky pitch
  void thickLine(int x0, int y0, int x1, int y1, const RGB& c) {
    line(x0, y0, x1, y1, c);
    line(x0 + 1, y0, x1 + 1, y1, c);
    line(x0, y0 + 1, x1, y1 + 1, c);
  }

  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2,
                    const RGB& c) {
    int minX = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int maxX = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int minY = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int maxY = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    for (int y = minY; y <= maxY; y++) {
      for (int x = minX; x <= maxX; x++) {
        // same-side test, winding-agnostic
        int s0 = (x1 - x0) * (y - y0) - (y1 - y0) * (x - x0);
        int s1 = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
        int s2 = (x0 - x2) * (y - y2) - (y0 - y2) * (x - x2);
        bool allNonNeg = s0 >= 0 && s1 >= 0 && s2 >= 0;
        bool allNonPos = s0 <= 0 && s1 <= 0 && s2 <= 0;
        if (allNonNeg || allNonPos) set(x, y, c);
      }
    }
  }

  void fillCircle(int cx, int cy, int r, const RGB& c) {
    for (int dy = -r; dy <= r; dy++)
      for (int dx = -r; dx <= r; dx++)
        if (dx * dx + dy * dy <= r * r) set(cx + dx, cy + dy, c);
  }

  void circle(int cx, int cy, int r, const RGB& c) {
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
      set(cx + x, cy + y, c); set(cx + y, cy + x, c);
      set(cx - y, cy + x, c); set(cx - x, cy + y, c);
      set(cx - x, cy - y, c); set(cx - y, cy - x, c);
      set(cx + y, cy - x, c); set(cx + x, cy - y, c);
      y++;
      if (err < 0) err += 2 * y + 1;
      else { x--; err += 2 * (y - x) + 1; }
    }
  }

 private:
  RGB scratch_;
  RGB scratchConst_;
};

}

#endif
