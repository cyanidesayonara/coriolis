// Classic three-wave plasma over the current palette.
#ifndef CORIOLIS_SCENE_PLASMA_H
#define CORIOLIS_SCENE_PLASMA_H

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class PlasmaScene : public Scene {
 public:
  const char* name() const { return "Plasma"; }

  uint32_t draw(Context& ctx) {
    uint8_t t = uint8_t(ctx.nowMs / 16);
    for (int y = 0; y < ctx.fb.height(); y++) {
      for (int x = 0; x < ctx.fb.width(); x++) {
        uint8_t v = sin8(uint8_t(x * 3 + t)) + sin8(uint8_t(y * 3 - t)) +
                    sin8(uint8_t((x + y) * 2 + t / 2));
        ctx.fb.set(x, y, ctx.palette->lookup(v));
      }
    }
    return 16;
  }
};

}

#endif
