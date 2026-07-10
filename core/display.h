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

 private:
  RGB scratch_;
  RGB scratchConst_;
};

}

#endif
