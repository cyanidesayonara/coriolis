// Scene Launcher — a categorized quick-picker for the 25 scenes.
// Grouped into CLOCKS, GUIDED FITNESS, GAMES, and AMBIENT ART.
// Up/Down changes category, Left/Right picks a scene, OK launches it.
#ifndef CORIOLIS_SCENE_LAUNCHER_H
#define CORIOLIS_SCENE_LAUNCHER_H

#include "../core/scene.h"
#include "../core/font.h"

namespace coriolis {

class SceneLauncher {
 public:
  struct Entry {
    const char* name;
    int sceneIndex;
  };

  struct Category {
    const char* title;
    int count;
    Entry entries[12];
  };

  SceneLauncher() {
    // 0: Clocks & Info
    categories_[0].title = "CLOCKS & INFO";
    categories_[0].count = 6;
    categories_[0].entries[0] = {"Digital", 0};
    categories_[0].entries[1] = {"Analog", 1};
    categories_[0].entries[2] = {"Word", 2};
    categories_[0].entries[3] = {"Weather", 3};
    categories_[0].entries[4] = {"Calendar", 4};
    categories_[0].entries[5] = {"Bounce", 19};

    // 1: Guided Fitness
    categories_[1].title = "GUIDED FITNESS";
    categories_[1].count = 4;
    categories_[1].entries[0] = {"Yoga", 9};
    categories_[1].entries[1] = {"Exercise", 10};
    categories_[1].entries[2] = {"Breathe", 11};
    categories_[1].entries[3] = {"Focus", 12};

    // 2: Games
    categories_[2].title = "GAMES";
    categories_[2].count = 4;
    categories_[2].entries[0] = {"Pong", 5};
    categories_[2].entries[1] = {"Snake", 6};
    categories_[2].entries[2] = {"Tetris", 7};
    categories_[2].entries[3] = {"Labyrinth", 8};

    // 3: Ambient Art
    categories_[3].title = "AMBIENT ART";
    categories_[3].count = 11;
    categories_[3].entries[0] = {"Coriolis", 16};
    categories_[3].entries[1] = {"Aurora", 24};
    categories_[3].entries[2] = {"Lava", 17};
    categories_[3].entries[3] = {"Spiro", 14};
    categories_[3].entries[4] = {"Mandala", 15};
    categories_[3].entries[5] = {"Fireplace", 23};
    categories_[3].entries[6] = {"Aquarium", 22};
    categories_[3].entries[7] = {"Starfield", 20};
    categories_[3].entries[8] = {"Life", 21};
    categories_[3].entries[9] = {"Rain", 18};
    categories_[3].entries[10] = {"GIFs", 13};
  }

  void open(int currentSceneIdx) {
    active_ = true;
    catIdx_ = 0;
    itemIdx_ = 0;
    for (int c = 0; c < CAT_COUNT; c++) {
      for (int i = 0; i < categories_[c].count; i++) {
        if (categories_[c].entries[i].sceneIndex == currentSceneIdx) {
          catIdx_ = c;
          itemIdx_ = i;
          return;
        }
      }
    }
  }

  void close() { active_ = false; }
  bool isActive() const { return active_; }

  int handleInput(Key k) {
    if (!active_) return -1;
    if (k == Key::Up) {
      catIdx_ = (catIdx_ + CAT_COUNT - 1) % CAT_COUNT;
      if (itemIdx_ >= categories_[catIdx_].count) itemIdx_ = 0;
      return -2;  // consumed
    }
    if (k == Key::Down) {
      catIdx_ = (catIdx_ + 1) % CAT_COUNT;
      if (itemIdx_ >= categories_[catIdx_].count) itemIdx_ = 0;
      return -2;
    }
    if (k == Key::Left) {
      int count = categories_[catIdx_].count;
      itemIdx_ = (itemIdx_ + count - 1) % count;
      return -2;
    }
    if (k == Key::Right) {
      int count = categories_[catIdx_].count;
      itemIdx_ = (itemIdx_ + 1) % count;
      return -2;
    }
    if (k == Key::Select) {
      active_ = false;
      return categories_[catIdx_].entries[itemIdx_].sceneIndex;
    }
    if (k == Key::Back) {
      active_ = false;
      return -1;
    }
    return -2;
  }

  void draw(Context& ctx) {
    if (!active_) return;
    const int W = ctx.fb.width(), H = ctx.fb.height();

    // Dark translucent backdrop
    for (int y = 0; y < H; y++) {
      for (int x = 0; x < W; x++) {
        ctx.fb.at(x, y).dim(32);
      }
    }

    // Top Header
    RGB titleCol = ctx.palette->lookupBright(0);
    font3x5::drawText(ctx.fb, "SCENE SELECT", 4, 3, 1, titleCol);
    ctx.fb.hLine(0, W - 1, 10, RGB(60, 60, 60));

    // Categories list on the left / upper section
    int cy = 15;
    for (int c = 0; c < CAT_COUNT; c++) {
      bool isSelCat = (c == catIdx_);
      RGB catCol = isSelCat ? RGB(255, 255, 255) : RGB(100, 100, 100);
      if (isSelCat) {
        ctx.fb.rect(2, cy + 1, 2, 4, ctx.palette->lookupBright(0));
      }
      font3x5::drawText(ctx.fb, categories_[c].title, 7, cy, 1, catCol);
      cy += 9;
    }

    ctx.fb.hLine(4, W - 5, cy + 2, RGB(55, 55, 55));
    cy += 7;

    // Items within the selected category
    const Category& curCat = categories_[catIdx_];
    int startY = cy;
    for (int i = 0; i < curCat.count; i++) {
      bool isSel = (i == itemIdx_);
      int iy = startY + (i % 6) * 10;
      int ix = (i < 6) ? 6 : (W / 2 + 2);

      if (isSel) {
        RGB bar = ctx.palette->lookupBright(0);
        bar.dim(75);
        ctx.fb.rect(ix - 2, iy - 1, (W / 2) - 6, 8, bar);
      }
      RGB itemCol = isSel ? RGB(255, 255, 255) : RGB(140, 140, 140);
      font3x5::drawText(ctx.fb, curCat.entries[i].name, ix + 1, iy, 1, itemCol);
    }

    // Bottom navigation hint
    const char* hint = "UP/DN:CAT  L/R:SCENE  OK:GO";
    int hw = font3x5::textWidth(hint, 1);
    font3x5::drawText(ctx.fb, hint, (W - hw) / 2, H - 9, 1, RGB(120, 120, 120));
  }

 private:
  static const int CAT_COUNT = 4;
  Category categories_[CAT_COUNT];
  int catIdx_ = 0;
  int itemIdx_ = 0;
  bool active_ = false;
};

}

#endif
