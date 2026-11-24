// The framebuffer every scene draws into. Backends (desktop simulator,
// Teensy/SmartMatrix) copy it to their real output once per frame — scenes
// never talk to hardware.
#ifndef CORIOLIS_DISPLAY_H
#define CORIOLIS_DISPLAY_H

#include <string.h>
#include <math.h>

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

  // alpha-blend c over pixel (x, y); alpha 0 = no-op, 255 = overwrite
  void blend(int x, int y, const RGB& c, uint8_t alpha) {
    if (alpha == 0 || !contains(x, y)) return;
    RGB& dst = px[y * WIDTH + x];
    if (alpha == 255) { dst = c; return; }
    dst.r = lerp8(dst.r, c.r, alpha);
    dst.g = lerp8(dst.g, c.g, alpha);
    dst.b = lerp8(dst.b, c.b, alpha);
  }

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

  // ~3px line for the beefier limbs
  void boldLine(int x0, int y0, int x1, int y1, const RGB& c) {
    thickLine(x0, y0, x1, y1, c);
    line(x0 - 1, y0, x1 - 1, y1, c);
    line(x0, y0 - 1, x1, y1 - 1, c);
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

  // anti-aliased filled circle in continuous pixel-center coordinates
  void fillCircleAA(float cx, float cy, float r, const RGB& c) {
    if (r <= 0.0f) return;
    int x0 = int(floorf(cx - r - 1.0f));
    int x1 = int(ceilf(cx + r + 1.0f));
    int y0 = int(floorf(cy - r - 1.0f));
    int y1 = int(ceilf(cy + r + 1.0f));
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= WIDTH) x1 = WIDTH - 1;
    if (y1 >= HEIGHT) y1 = HEIGHT - 1;
    float rOut2 = (r + 1.0f) * (r + 1.0f);
    for (int y = y0; y <= y1; y++) {
      float dy = (y + 0.5f) - cy;
      for (int x = x0; x <= x1; x++) {
        float dx = (x + 0.5f) - cx;
        float d2 = dx * dx + dy * dy;
        if (d2 >= rOut2) continue;
        float d = sqrtf(d2);
        float cov = r + 0.5f - d;
        if (cov <= 0.0f) continue;
        if (cov >= 1.0f) set(x, y, c);
        else blend(x, y, c, uint8_t(cov * 255.0f + 0.5f));
      }
    }
  }

  // anti-aliased ring arc around (cx, cy), angles in radians (0 = +x, +pi/2 = +y)
  void drawArc(int cx, int cy, int r, int thickness, float startAngle,
               float sweepAngle, const RGB& c) {
    if (sweepAngle <= 0.0f || thickness <= 0 || r <= 0) return;
    const float twoPi = 6.28318530718f;
    bool full = sweepAngle >= twoPi - 0.001f;
    float halfT = thickness * 0.5f;
    float rMin = float(r) - halfT - 1.0f;
    float rMax = float(r) + halfT + 1.0f;
    if (rMin < 0.0f) rMin = 0.0f;
    float rMin2 = rMin * rMin;
    float rMax2 = rMax * rMax;

    int pad = int(ceilf(rMax));
    int x0 = cx - pad, x1 = cx + pad;
    int y0 = cy - pad, y1 = cy + pad;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= WIDTH) x1 = WIDTH - 1;
    if (y1 >= HEIGHT) y1 = HEIGHT - 1;

    for (int y = y0; y <= y1; y++) {
      float dy = float(y - cy);
      for (int x = x0; x <= x1; x++) {
        float dx = float(x - cx);
        float d2 = dx * dx + dy * dy;
        if (d2 < rMin2 || d2 > rMax2) continue;
        float d = sqrtf(d2);
        float radCov = halfT + 0.5f - fabsf(d - float(r));
        if (radCov <= 0.0f) continue;
        if (radCov > 1.0f) radCov = 1.0f;

        float angCov = 1.0f;
        if (!full) {
          float a = atan2f(dy, dx) - startAngle;
          while (a < 0.0f) a += twoPi;
          while (a >= twoPi) a -= twoPi;
          if (a > sweepAngle) {
            float dEnd = (a - sweepAngle) * d;
            float dStart = (twoPi - a) * d;
            float out = dEnd < dStart ? dEnd : dStart;
            if (out >= 1.0f) continue;
            angCov = 1.0f - out;
          }
        }
        float cov = radCov * angCov;
        if (cov >= 1.0f) set(x, y, c);
        else blend(x, y, c, uint8_t(cov * 255.0f + 0.5f));
      }
    }
  }

 private:
  RGB scratch_;
  RGB scratchConst_;
};

// scale brightness down only when a frame's total LED drive exceeds the
// current budget (e.g., full-white flashes), leaving normal scenes untouched
inline uint8_t powerLimitedBrightness(const FrameBuffer& fb, uint8_t requested,
                                      uint32_t maxMilliamps,
                                      uint32_t fullWhiteMilliamps = 32000) {
  if (maxMilliamps == 0 || requested == 0 || fullWhiteMilliamps == 0)
    return requested;
  uint32_t sum = 0;
  for (int i = 0; i < PIXELS; i++) {
    sum += fb.px[i].r + fb.px[i].g + fb.px[i].b;
  }
  if (sum == 0) return requested;
  uint64_t denom = uint64_t(PIXELS) * 765u * 255u;
  uint64_t estMa = (uint64_t(sum) * requested * fullWhiteMilliamps) / denom;
  if (estMa <= maxMilliamps) return requested;
  uint32_t scaled = uint32_t((uint64_t(maxMilliamps) * denom) /
                             (uint64_t(sum) * fullWhiteMilliamps));
  if (scaled < 1) return 1;
  return scaled > requested ? requested : uint8_t(scaled);
}

}

#endif
