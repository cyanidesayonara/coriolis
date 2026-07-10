// 16-anchor color palettes with interpolated lookup, in the spirit of the
// palettes the Borealis patterns were designed around.
#ifndef CORIOLIS_PALETTE_H
#define CORIOLIS_PALETTE_H

#include "color.h"

namespace coriolis {

struct Palette {
  RGB entries[16];
  const char* name;

  // index 0..255 sweeps the whole palette with blending
  RGB lookup(uint8_t index, uint8_t brightness = 255) const {
    uint8_t slot = index >> 4;
    uint8_t frac = (index & 0x0F) << 4;
    RGB c = lerpColor(entries[slot], entries[(slot + 1) & 15], frac);
    if (brightness != 255) c.dim(brightness);
    return c;
  }
};

namespace palettes {

inline Palette gradient(const char* name, RGB a, RGB b, RGB c, RGB d) {
  Palette p;
  p.name = name;
  const RGB anchor[4] = {a, b, c, d};
  for (int i = 0; i < 16; i++) {
    float t = i / 15.0f * 3.0f;
    int seg = int(t) > 2 ? 2 : int(t);
    p.entries[i] =
        lerpColor(anchor[seg], anchor[seg + 1], uint8_t((t - seg) * 255));
  }
  return p;
}

inline Palette rainbow() {
  Palette p;
  p.name = "Rainbow";
  for (int i = 0; i < 16; i++) p.entries[i] = hsv(uint8_t(i * 16), 255, 255);
  return p;
}

inline Palette heat() {
  return gradient("Heat", RGB(0x000000), RGB(0xB00000), RGB(0xFFA500),
                  RGB(0xFFFFCC));
}

inline Palette ocean() {
  return gradient("Ocean", RGB(0x001030), RGB(0x0059B3), RGB(0x00B386),
                  RGB(0x99FFEE));
}

inline Palette forest() {
  return gradient("Forest", RGB(0x001800), RGB(0x228B22), RGB(0x9ACD32),
                  RGB(0xE0FFC0));
}

inline Palette ice() {
  return gradient("Ice", RGB(0x000000), RGB(0x0000A0), RGB(0x00CCCC),
                  RGB(0xFFFFFF));
}

inline Palette grayscale() {
  return gradient("Gray", RGB(0x000000), RGB(0x555555), RGB(0xAAAAAA),
                  RGB(0xFFFFFF));
}

static const int COUNT = 6;

// stable set for cycling in scenes/menus
inline const Palette& byIndex(int i) {
  static const Palette all[COUNT] = {rainbow(), heat(),      ocean(),
                                     forest(),  ice(),       grayscale()};
  if (i < 0) i = 0;
  if (i >= COUNT) i = COUNT - 1;
  return all[i];
}

}

}

#endif
