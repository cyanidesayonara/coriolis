// Self-contained color type and operations. No FastLED dependency: the
// Teensy 4.1 has an FPU and 1MB of RAM, so Coriolis doesn't need borrowed
// fixed-point tricks — plain code that reads well and runs everywhere.
#ifndef CORIOLIS_COLOR_H
#define CORIOLIS_COLOR_H

#include <stdint.h>

namespace coriolis {

inline uint8_t scale8(uint8_t value, uint8_t scale) {
  return (uint16_t(value) * scale) >> 8;
}

inline uint8_t qadd8(uint8_t a, uint8_t b) {
  int sum = a + b;
  return sum > 255 ? 255 : uint8_t(sum);
}

inline uint8_t qsub8(uint8_t a, uint8_t b) {
  int diff = a - b;
  return diff < 0 ? 0 : uint8_t(diff);
}

inline uint8_t lerp8(uint8_t a, uint8_t b, uint8_t frac) {
  return a + scale8(uint8_t(b - a), frac);  // wraps correctly for b < a
}

struct RGB {
  uint8_t r, g, b;

  RGB() : r(0), g(0), b(0) {}
  RGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
  explicit RGB(uint32_t hex)
      : r((hex >> 16) & 0xFF), g((hex >> 8) & 0xFF), b(hex & 0xFF) {}

  RGB& dim(uint8_t scale) {
    r = scale8(r, scale);
    g = scale8(g, scale);
    b = scale8(b, scale);
    return *this;
  }

  RGB& add(const RGB& o) {
    r = qadd8(r, o.r);
    g = qadd8(g, o.g);
    b = qadd8(b, o.b);
    return *this;
  }

  bool isBlack() const { return r == 0 && g == 0 && b == 0; }

  bool operator==(const RGB& o) const {
    return r == o.r && g == o.g && b == o.b;
  }
};

inline RGB lerpColor(const RGB& a, const RGB& b, uint8_t frac) {
  return RGB(lerp8(a.r, b.r, frac), lerp8(a.g, b.g, frac),
             lerp8(a.b, b.b, frac));
}

// hue/sat/val in 0..255, spectrum-style mapping
inline RGB hsv(uint8_t h, uint8_t s, uint8_t v) {
  if (s == 0) return RGB(v, v, v);

  uint8_t region = h / 43;               // six 42.5-wide regions
  uint8_t rem = (h - region * 43) * 6;   // position within region, 0..255

  uint8_t p = scale8(v, 255 - s);
  uint8_t q = scale8(v, 255 - scale8(s, rem));
  uint8_t t = scale8(v, 255 - scale8(s, 255 - rem));

  switch (region) {
    case 0: return RGB(v, t, p);
    case 1: return RGB(q, v, p);
    case 2: return RGB(p, v, t);
    case 3: return RGB(p, q, v);
    case 4: return RGB(t, p, v);
    default: return RGB(v, p, q);
  }
}

}

#endif
