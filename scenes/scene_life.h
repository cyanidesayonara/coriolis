// Conway's Game of Life — a Borealis pattern reborn. Cells live and die by
// the classic rules; newly born cells glow brighter than survivors. When the
// board stalls or thins out, it reseeds with a fresh soup.
#ifndef CORIOLIS_SCENE_LIFE_H
#define CORIOLIS_SCENE_LIFE_H

#include <string.h>

#include "../core/scene.h"
#include "../core/math8.h"

namespace coriolis {

class LifeScene : public Scene {
 public:
  const char* name() const { return "Life"; }

  void start(Context& ctx) {
    gw_ = ctx.fb.width() / CELL;
    gh_ = ctx.fb.height() / CELL;
    if (gw_ > MAXG) gw_ = MAXG;
    if (gh_ > MAXG) gh_ = MAXG;
    reseed();
    hue_ = 40;
    stallCount_ = 0;
    lastStep_ = lastHue_ = ctx.nowMs;
  }

  uint32_t draw(Context& ctx) {
    if (ctx.nowMs - lastStep_ >= 110) { lastStep_ = ctx.nowMs; step(); }
    if (ctx.nowMs - lastHue_ >= 60) { lastHue_ = ctx.nowMs; hue_ += 1; }

    ctx.fb.clear();
    RGB live = ctx.palette->lookupBright(hue_);
    RGB born = ctx.palette->lookupBright(uint8_t(hue_ + 40));
    for (int y = 0; y < gh_; y++)
      for (int x = 0; x < gw_; x++)
        if (cur_[x][y])
          ctx.fb.rect(x * CELL, y * CELL, CELL - 1, CELL - 1,
                      born_[x][y] ? born : live);

    return 30;
  }

 private:
  static const int CELL = 3;
  static const int MAXG = 48;

  bool cur_[MAXG][MAXG];
  bool next_[MAXG][MAXG];
  bool born_[MAXG][MAXG];
  int gw_, gh_, population_, stallCount_;
  uint8_t hue_;
  uint32_t lastStep_, lastHue_;

  void reseed() {
    for (int y = 0; y < gh_; y++)
      for (int x = 0; x < gw_; x++) {
        cur_[x][y] = random8() < 75;  // ~30% soup
        born_[x][y] = false;
      }
    population_ = -1;
    stallCount_ = 0;
  }

  void step() {
    int pop = 0;
    for (int y = 0; y < gh_; y++) {
      for (int x = 0; x < gw_; x++) {
        int n = 0;
        for (int dy = -1; dy <= 1; dy++)
          for (int dx = -1; dx <= 1; dx++) {
            if (!dx && !dy) continue;
            int nx = (x + dx + gw_) % gw_, ny = (y + dy + gh_) % gh_;  // torus
            if (cur_[nx][ny]) n++;
          }
        bool alive = cur_[x][y] ? (n == 2 || n == 3) : (n == 3);
        next_[x][y] = alive;
        born_[x][y] = alive && !cur_[x][y];
        if (alive) pop++;
      }
    }
    memcpy(cur_, next_, sizeof(cur_));

    // reseed on death or a long stall
    if (pop == population_) stallCount_++;
    else stallCount_ = 0;
    population_ = pop;
    if (pop < gw_ * gh_ / 40 || stallCount_ > 24) reseed();
  }
};

}

#endif
