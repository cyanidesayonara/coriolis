// Fireplace — a brick surround with an arched firebox and a flue. Flames
// are uneven, licking higher in slow travelling hot spots; embers fly well
// above the fire; smoke rises through the arch and up the flue. Never
// themed: fire looks like fire.
// (On the device the DFPlayer's crackle loop plays under this, and each
// spark "pop" triggers a pop sound — see spawnPop.)
#ifndef CORIOLIS_SCENE_FIRE_H
#define CORIOLIS_SCENE_FIRE_H

#include <string.h>
#include <math.h>

#include "../core/scene.h"
#include "../core/math8.h"
#include "../core/settings.h"

namespace coriolis {

class FireScene : public Scene {
 public:
  explicit FireScene(Settings& settings) : settings_(settings) {}

  const char* name() const { return "Fireplace"; }

  void start(Context& ctx) {
    memset(heat_, 0, sizeof(heat_));
    for (int i = 0; i < MAX_PARTICLES; i++) particles_[i].life = 0;
    geometry(ctx);
  }

  uint32_t draw(Context& ctx) {
    geometry(ctx);
    phase_ += 3;

    // --- simulate flames, column by column, inside the firebox ----------
    int fireTop = archApexY_ + 2;
    float t = phase_ * 0.03f;
    for (int x = ix0_; x <= ix1_; x++) {
      // cool, harder with altitude so the fire tapers to tips
      for (int y = fireTop; y <= hearthY_; y++) {
        int altitude = hearthY_ - y;
        uint8_t cool = uint8_t(random8(0, 9) + altitude * 55 / 100);
        heat_[idx(x, y)] = qsub8(heat_[idx(x, y)], cool);
      }
      // heat rises and diffuses upward
      for (int y = fireTop; y < hearthY_ - 1; y++) {
        heat_[idx(x, y)] =
            uint8_t((heat_[idx(x, y + 1)] * 2 + heat_[idx(x, y + 2)]) / 3);
      }
      // feed the base unevenly: two travelling waves make hot spots that
      // drift, so flames lick higher in some places and lower in others
      float wave = sinf(x * 0.20f + t) * sinf(x * 0.07f - t * 0.6f);
      float fuel = 0.45f + 0.55f * (wave * 0.5f + 0.5f);
      if (random8() < 170) {
        int y = hearthY_ - randomInt(2);
        uint8_t add = uint8_t(random8(50, 110) + fuel * 130);
        heat_[idx(x, y)] = qadd8(heat_[idx(x, y)], add);
      }
    }

    // --- render ---------------------------------------------------------
    ctx.fb.clear();

    for (int y = fireTop; y <= hearthY_; y++) {
      for (int x = ix0_; x <= ix1_; x++) {
        uint8_t v = heat_[idx(x, y)];
        if (v > 5 && insideOpening(x, y))
          ctx.fb.set(x, y, palettes::byIndex(1).lookup(scale8(v, 235)));
      }
    }

    drawLogs(ctx);
    updateParticles(ctx);
    drawSurround(ctx);

    return 33;
  }

 private:
  static const int MAX_PARTICLES = 28;

  struct Particle {
    float x, y, vx, vy;
    int life, maxLife;
    uint8_t type;  // 0 spark, 1 smoke
  };

  Settings& settings_;
  uint8_t heat_[PIXELS];
  Particle particles_[MAX_PARTICLES];
  uint32_t phase_ = 0;

  // geometry, recomputed each frame (cheap, keeps it size-correct)
  int SL_, SR_, hearthY_, ix0_, ix1_, archSpringY_, archR_, archCX_;
  int archApexY_, flueHalf_, surroundTop_, chimL_, chimR_;

  static int idx(int x, int y) { return y * WIDTH + x; }

  void geometry(Context& ctx) {
    const int w = ctx.fb.width();
    const int h = ctx.fb.height();
    SL_ = w * 15 / 100;
    SR_ = w * 85 / 100;
    hearthY_ = h - h * 7 / 100;
    ix0_ = w * 30 / 100;
    ix1_ = w * 70 / 100;
    archCX_ = w / 2;
    archR_ = (ix1_ - ix0_) / 2;
    archSpringY_ = h * 62 / 100;
    archApexY_ = archSpringY_ - archR_;
    flueHalf_ = w * 5 / 100;
    surroundTop_ = h * 30 / 100;
    chimL_ = w * 34 / 100;
    chimR_ = w * 66 / 100;
  }

  // the keyhole: rectangular firebox + arch + a thin flue up to the top
  bool insideOpening(int x, int y) const {
    if (x >= ix0_ && x <= ix1_ && y >= archSpringY_ && y <= hearthY_)
      return true;
    int dx = x - archCX_, dy = y - archSpringY_;
    if (y < archSpringY_ && dx * dx + dy * dy <= archR_ * archR_) return true;
    if (y < archSpringY_ && (dx < 0 ? -dx : dx) <= flueHalf_) return true;
    return false;
  }

  bool inBrickRegion(int x, int y) const {
    bool mainSurround = x >= SL_ && x <= SR_ && y >= surroundTop_ && y <= hearthY_;
    bool chimney = x >= chimL_ && x <= chimR_ && y < surroundTop_;
    return (mainSurround || chimney) && !insideOpening(x, y);
  }

  void drawSurround(Context& ctx) {
    RGB mortar(55, 40, 35);
    const int bw = 8, bh = 4;
    for (int y = 0; y <= hearthY_; y++) {
      for (int x = SL_; x <= SR_; x++) {
        if (!inBrickRegion(x, y)) continue;
        int row = y / bh;
        int off = (row & 1) ? bw / 2 : 0;
        bool seam = (y % bh == 0) || ((x + off) % bw == 0);
        if (seam) {
          ctx.fb.set(x, y, mortar);
        } else {
          // slight per-brick shade variation for texture
          int cell = ((x + off) / bw * 7 + row * 13) & 3;
          uint8_t base = uint8_t(120 + cell * 8);
          ctx.fb.set(x, y, RGB(base, uint8_t(base * 55 / 100),
                               uint8_t(base * 35 / 100)));
        }
      }
    }

    // mantel shelf: a lighter stone bar protruding over the firebox
    int my = surroundTop_;
    RGB stone(150, 140, 128);
    ctx.fb.rect(SL_ - 3, my - 2, (SR_ - SL_) + 6, 3, stone);

    // hearth floor
    RGB hearth(90, 78, 70);
    ctx.fb.rect(SL_ - 3, hearthY_, (SR_ - SL_) + 6, ctx.fb.height() - hearthY_,
                hearth);
  }

  void drawLogs(Context& ctx) {
    RGB bark(92, 55, 25), barkLight(120, 74, 36), cut(170, 120, 70);
    int y0 = hearthY_ - 4;
    // two rounded-end logs crossed over the fire base
    for (int i = 0; i < 2; i++) {
      int lx = i == 0 ? ix0_ + 2 : ix0_ + (ix1_ - ix0_) / 4;
      int lw = i == 0 ? (ix1_ - ix0_) - 4 : (ix1_ - ix0_) / 2;
      int ly = i == 0 ? y0 : y0 - 3;
      RGB c = i == 0 ? bark : barkLight;
      ctx.fb.rect(lx, ly, lw, 3, c);
      ctx.fb.fillCircle(lx, ly + 1, 1, cut);
      ctx.fb.fillCircle(lx + lw, ly + 1, 1, cut);
    }
  }

  void spawn(uint8_t type, float x, float y, float vx, float vy, int life) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (particles_[i].life <= 0) {
        particles_[i] = {x, y, vx, vy, life, life, type};
        return;
      }
    }
  }

  void updateParticles(Context& ctx) {
    float baseX = float(archCX_);
    // embers off the logs
    if (settings_.fireSparks) {
      if (random8() < 7) {
        float x = baseX + randomInt(ix1_ - ix0_) - (ix1_ - ix0_) / 2;
        spawn(0, x, float(hearthY_ - 3), (random8(3) - 1) * 0.3f,
              -(1.0f + random8(90) / 100.0f), 30 + random8(24));
      }
      if (random8() < 2) {  // a pop throws several — DFPlayer pop trigger here
        for (int k = 0; k < 4; k++)
          spawn(0, baseX + randomInt(20) - 10, float(hearthY_ - 4),
                (random8(5) - 2) * 0.4f, -(1.3f + random8(120) / 100.0f),
                34 + random8(26));
      }
    }
    // smoke off the flame tips, drifting up the flue
    if (random8() < 22) {
      spawn(1, baseX + randomInt(archR_) - archR_ / 2, float(archApexY_ + 4),
            (random8(3) - 1) * 0.2f, -(0.5f + random8(60) / 100.0f),
            60 + random8(40));
    }

    for (int i = 0; i < MAX_PARTICLES; i++) {
      Particle& p = particles_[i];
      if (p.life <= 0) continue;
      p.life--;
      p.x += p.vx;
      p.y += p.vy;
      p.vx += (random8(3) - 1) * 0.06f;  // flicker/drift

      int xi = int(p.x), yi = int(p.y);
      if (yi < 0 || !insideOpening(xi, yi)) { p.life = 0; continue; }

      if (p.type == 0) {
        RGB c = p.life > p.maxLife / 2 ? RGB(255, 220, 130) : RGB(210, 90, 25);
        ctx.fb.set(xi, yi, c);
      } else {
        // smoke greys and thins as it rises
        uint8_t g = uint8_t(30 + 60 * p.life / p.maxLife);
        ctx.fb.set(xi, yi, RGB(g, g, g));
        if (p.life > p.maxLife * 3 / 4) ctx.fb.set(xi + 1, yi, RGB(g, g, g));
      }
    }
  }
};

}

#endif
