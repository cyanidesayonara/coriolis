// Fireplace — not an inferno. Flames rise from a pair of logs and die out
// well below the middle of the screen; embers pop off the wood now and
// then and drift up. Never themed: fire looks like fire.
// (On the device, the DFPlayer's crackle loop and the occasional pop sound
// hook into spawnSparks below.)
#ifndef CORIOLIS_SCENE_FIRE_H
#define CORIOLIS_SCENE_FIRE_H

#include <string.h>

#include "../core/scene.h"
#include "../core/math8.h"
#include "../core/settings.h"

namespace coriolis {

class FireScene : public Scene {
 public:
  explicit FireScene(Settings& settings) : settings_(settings) {}

  const char* name() const { return "Fireplace"; }

  void start(Context&) {
    memset(heat_, 0, sizeof(heat_));
    for (int i = 0; i < MAX_SPARKS; i++) sparks_[i].life = 0;
  }

  uint32_t draw(Context& ctx) {
    const int w = ctx.fb.width();
    const int h = ctx.fb.height();
    const int baseY = h - 7;            // top of the log pile
    const int flameCeil = h * 45 / 100; // flames may not rise above this

    for (int x = 0; x < w; x++) {
      // cool every cell — harder the higher it sits above the logs, which
      // is what keeps this a fireplace
      for (int y = flameCeil; y <= baseY; y++) {
        int altitude = baseY - y;
        uint8_t cool = uint8_t(random8(0, 12) + altitude * 3 / 2);
        heat_[idx(x, y)] = qsub8(heat_[idx(x, y)], cool);
      }

      // heat drifts upward, diffusing
      for (int y = flameCeil; y < baseY - 1; y++) {
        heat_[idx(x, y)] =
            uint8_t((heat_[idx(x, y + 1)] + heat_[idx(x, y + 2)] * 2) / 3);
      }

      // the fire feeds along the log line, hotter near the middle
      if (random8() < 110) {
        int mid = w / 2;
        int dist = x > mid ? x - mid : mid - x;
        if (dist < w * 3 / 10) {
          int y = baseY - randomInt(2);
          heat_[idx(x, y)] = qadd8(heat_[idx(x, y)], random8(120, 220));
        }
      }
    }

    // render flames through the fixed heat ramp
    ctx.fb.clear();
    for (int y = flameCeil; y <= baseY; y++) {
      for (int x = 0; x < w; x++) {
        uint8_t v = heat_[idx(x, y)];
        if (v > 4) {
          ctx.fb.set(x, y, palettes::byIndex(1).lookup(scale8(v, 235)));
        }
      }
    }

    drawLogs(ctx, w, h);

    if (settings_.fireSparks) updateSparks(ctx, w, baseY);

    return 33;
  }

 private:
  static const int MAX_SPARKS = 6;

  struct Spark {
    float x, y, vy;
    int life;
  };

  Settings& settings_;
  uint8_t heat_[PIXELS];
  Spark sparks_[MAX_SPARKS];

  static int idx(int x, int y) { return y * WIDTH + x; }

  void drawLogs(Context& ctx, int w, int h) {
    RGB bark(92, 55, 25);
    RGB barkLight(130, 82, 40);
    RGB cut(165, 110, 55);

    // bottom log, full width of the hearth
    int y0 = h - 5;
    ctx.fb.rect(w / 5, y0, w * 3 / 5, 3, bark);
    ctx.fb.rect(w / 5, y0, 2, 3, cut);
    ctx.fb.rect(w * 4 / 5 - 2, y0, 2, 3, cut);

    // upper log, shorter, resting across
    int y1 = h - 8;
    ctx.fb.rect(w * 3 / 10, y1, w * 2 / 5, 3, barkLight);
    ctx.fb.rect(w * 3 / 10, y1, 2, 3, cut);
    ctx.fb.rect(w * 7 / 10 - 2, y1, 2, 3, cut);
  }

  void updateSparks(Context& ctx, int w, int baseY) {
    // occasional single embers; rarely a "pop" throws a few at once —
    // on the device the pop is also the DFPlayer's crackle trigger
    int toSpawn = 0;
    if (random8() < 5) toSpawn = 1;
    if (random8() < 2) toSpawn = 3;

    for (int i = 0; i < MAX_SPARKS && toSpawn > 0; i++) {
      if (sparks_[i].life <= 0) {
        sparks_[i].x = float(w / 2 + int(random8(0, uint8_t(w / 3))) - w / 6);
        sparks_[i].y = float(baseY - 2);
        sparks_[i].vy = -(0.4f + random8(60) / 100.0f);
        sparks_[i].life = 12 + random8(18);
        toSpawn--;
      }
    }

    for (int i = 0; i < MAX_SPARKS; i++) {
      Spark& s = sparks_[i];
      if (s.life <= 0) continue;
      s.life--;
      s.y += s.vy;
      s.x += (random8(3) - 1) * 0.4f;  // flickery sideways drift

      RGB emberColor = s.life > 8 ? RGB(255, 220, 130) : RGB(200, 90, 30);
      ctx.fb.set(int(s.x), int(s.y), emberColor);
    }
  }
};

}

#endif
