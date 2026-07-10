// 8-bit wave/random helpers in the FastLED idiom (phase and value both
// 0..255), implemented with plain float math — every Coriolis target has
// an FPU or is a desktop.
#ifndef CORIOLIS_MATH8_H
#define CORIOLIS_MATH8_H

#include <stdint.h>
#include <math.h>

#include "color.h"

namespace coriolis {

static const float TWO_PI_F = 6.28318530718f;

inline uint8_t sin8(uint8_t theta) {
  return uint8_t(sinf(theta * (TWO_PI_F / 256.0f)) * 127.5f + 128.0f);
}

inline uint8_t cos8(uint8_t theta) {
  return uint8_t(cosf(theta * (TWO_PI_F / 256.0f)) * 127.5f + 128.0f);
}

inline uint8_t triwave8(uint8_t in) {
  return in < 128 ? in * 2 : (255 - in) * 2;
}

// map a 0..255 wave value into [lo, hi]
inline uint8_t rescale8(uint8_t value, uint8_t lo, uint8_t hi) {
  return lo + scale8(value, uint8_t(hi - lo));
}

// sawtooth phase at the given bpm, 0..255 per beat
inline uint8_t beat8(uint16_t bpm, uint32_t nowMs) {
  return uint8_t((nowMs * bpm * 280u) >> 16);
}

inline uint8_t beatsin8(uint16_t bpm, uint32_t nowMs, uint8_t lo = 0,
                        uint8_t hi = 255, uint8_t phase = 0) {
  return rescale8(sin8(beat8(bpm, nowMs) + phase), lo, hi);
}

// xorshift PRNG — deterministic given a seed, which makes simulator
// sessions reproducible when needed
inline uint32_t& randomState() {
  static uint32_t state = 0x2545F491;
  return state;
}

inline void randomSeed32(uint32_t seed) {
  randomState() = seed ? seed : 1;
}

inline uint32_t random32() {
  uint32_t& s = randomState();
  s ^= s << 13;
  s ^= s >> 17;
  s ^= s << 5;
  return s;
}

inline uint8_t random8() { return uint8_t(random32()); }

inline uint8_t random8(uint8_t below) {
  return below ? uint8_t(random32() % below) : 0;
}

inline uint8_t random8(uint8_t lo, uint8_t hi) {
  return hi <= lo ? lo : uint8_t(lo + random32() % (hi - lo));
}

inline int randomInt(int below) { return below > 0 ? int(random32() % uint32_t(below)) : 0; }

}

#endif
