// Scene Launcher — the categorized quick-picker opened by the remote's Menu
// button. Four icon tabs across the top (Clocks, Fitness, Games, Art),
// left/right switches tab, up/down walks the list, OK launches. The
// selection pill and the tab marker glide to their targets instead of
// jumping. Scenes are matched by name, so reordering the rotation never
// breaks the menu; anything unlisted lands under Art.
#ifndef CORIOLIS_SCENE_LAUNCHER_H
#define CORIOLIS_SCENE_LAUNCHER_H

#include <string.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/font5x7.h"

namespace coriolis {

class SceneLauncher {
 public:
  static const int CAT_COUNT = 4;
  static const int MAX_ITEMS = 16;

  // resolve the category tables against the actual scene list
  void attach(Scene* const* scenes, int count) {
    struct Def { const char* scene; const char* label; };
    static const Def clocks[] = {
        {"Clock", "Digital"}, {"Analog", "Analog"}, {"Word Clock", "Word clock"},
        {"Bounce", "Bouncing"}, {"Weather", "Weather"}, {"Calendar", "Calendar"}};
    static const Def fitness[] = {
        {"Yoga", "Yoga"}, {"Exercise", "Exercise"}, {"Breathe", "Breathe"},
        {"Focus", "Focus timer"}};
    static const Def games[] = {
        {"Tetris", "Tetris"}, {"Snake", "Snake"}, {"Pong", "Pong"},
        {"Labyrinth", "Labyrinth"}};
    static const Def art[] = {
        {"Coriolis", "Coriolis"}, {"Aurora", "Aurora"}, {"Fireplace", "Fireplace"},
        {"Aquarium", "Aquarium"}, {"Lava", "Lava lamp"}, {"Spiro", "Spirograph"},
        {"Mandala", "Mandala"}, {"Starfield", "Starfield"}, {"Rain", "Digital rain"},
        {"Life", "Game of life"}, {"Gifs", "GIF player"}};
    const Def* tables[CAT_COUNT] = {clocks, fitness, games, art};
    const int sizes[CAT_COUNT] = {int(sizeof(clocks) / sizeof(clocks[0])),
                                  int(sizeof(fitness) / sizeof(fitness[0])),
                                  int(sizeof(games) / sizeof(games[0])),
                                  int(sizeof(art) / sizeof(art[0]))};
    bool placed[64] = {false};
    for (int c = 0; c < CAT_COUNT; c++) {
      cats_[c].count = 0;
      for (int i = 0; i < sizes[c]; i++) {
        for (int s = 0; s < count && s < 64; s++) {
          if (strcmp(scenes[s]->name(), tables[c][i].scene) == 0) {
            add(c, tables[c][i].label, s);
            placed[s] = true;
            break;
          }
        }
      }
    }
    for (int s = 0; s < count && s < 64; s++)
      if (!placed[s]) add(3, scenes[s]->name(), s);
  }

  void open(int currentScene) {
    active_ = true;
    cat_ = 0;
    item_ = 0;
    for (int c = 0; c < CAT_COUNT; c++)
      for (int i = 0; i < cats_[c].count; i++)
        if (cats_[c].items[i].scene == currentScene) { cat_ = c; item_ = i; }
    pillY_ = -1.0f;  // snap on the first frame
    tabX_ = -1.0f;
    scroll_ = 0;
  }

  void close() { active_ = false; }
  bool isActive() const { return active_; }

  // >= 0: launch that scene; -1: closed without choosing; -2: consumed
  int handleInput(Key k) {
    if (!active_) return -1;
    switch (k) {
      case Key::Left:
        cat_ = (cat_ + CAT_COUNT - 1) % CAT_COUNT;
        item_ = 0;
        scroll_ = 0;
        return -2;
      case Key::Right:
        cat_ = (cat_ + 1) % CAT_COUNT;
        item_ = 0;
        scroll_ = 0;
        return -2;
      case Key::Up:
        if (cats_[cat_].count) item_ = (item_ + cats_[cat_].count - 1) % cats_[cat_].count;
        return -2;
      case Key::Down:
        if (cats_[cat_].count) item_ = (item_ + 1) % cats_[cat_].count;
        return -2;
      case Key::Select:
        if (!cats_[cat_].count) return -2;
        active_ = false;
        return cats_[cat_].items[item_].scene;
      case Key::Back:
        active_ = false;
        return -1;
      default:
        return -2;
    }
  }

  static RGB accent(int cat) {
    static const RGB a[CAT_COUNT] = {RGB(255, 184, 70), RGB(255, 120, 105),
                                     RGB(95, 225, 140), RGB(165, 140, 255)};
    return a[cat % CAT_COUNT];
  }

  static const char* title(int cat) {
    static const char* t[CAT_COUNT] = {"Clocks & Info", "Fitness", "Games",
                                       "Ambient Art"};
    return t[cat % CAT_COUNT];
  }

  // draws over whatever is already in the framebuffer (the shell puts a
  // dimmed still of the paused scene there)
  void draw(Context& ctx) {
    if (!active_) return;
    FrameBuffer& fb = ctx.fb;
    const int W = fb.width(), H = fb.height();
    const bool compact = H < 100;
    const RGB acc = accent(cat_);

    // --- tabs: four icons, the active one lit in its accent color
    const int iconY = 4, slot = W / CAT_COUNT;
    for (int c = 0; c < CAT_COUNT; c++) {
      int ix = c * slot + (slot - ICON) / 2;
      RGB col = c == cat_ ? accent(c) : RGB(70, 70, 76);
      drawIcon(fb, c, ix, iconY, col);
    }
    float tabTarget = float(cat_ * slot + slot / 2);
    tabX_ = tabX_ < 0 ? tabTarget : tabX_ + (tabTarget - tabX_) * 0.35f;
    int tx = int(tabX_ + 0.5f);
    fb.rect(tx - 6, iconY + ICON + 2, 13, 1, acc);

    // --- category title
    int titleY = iconY + ICON + 6;
    if (!compact) {
      font5x7::drawCentered(fb, title(cat_), W / 2, titleY, 1, RGB(235, 235, 240));
      titleY += 11;
    } else {
      titleY += 1;
    }

    // --- the list, scrolled so the selection stays in view
    const int rowH = compact ? 9 : 11;
    const int listTop = titleY + 1;
    const int footerH = compact ? 0 : 9;
    int visible = (H - listTop - footerH - 2) / rowH;
    if (visible < 1) visible = 1;
    const Category& cat = cats_[cat_];
    if (item_ < scroll_) scroll_ = item_;
    if (item_ >= scroll_ + visible) scroll_ = item_ - visible + 1;

    float pillTarget = float(listTop + (item_ - scroll_) * rowH);
    pillY_ = pillY_ < 0 ? pillTarget : pillY_ + (pillTarget - pillY_) * 0.4f;
    int py = int(pillY_ + 0.5f);
    RGB pill = acc;
    pill.dim(70);
    roundRect(fb, 6, py, W - 12, rowH - 1, pill);
    fb.rect(6, py + 2, 2, rowH - 5, acc);  // accent edge on the pill

    for (int v = 0; v < visible && scroll_ + v < cat.count; v++) {
      int i = scroll_ + v;
      int y = listTop + v * rowH + (rowH - 1 - font5x7::GLYPH_H) / 2;
      bool sel = i == item_;
      RGB col = sel ? RGB(255, 255, 255) : RGB(150, 150, 158);
      font5x7::drawText(fb, cat.items[i].label, 13, y, 1, col);
    }

    // scroll hints when the list overflows
    if (scroll_ > 0) arrow(fb, W - 9, listTop - 2, true, RGB(120, 120, 130));
    if (scroll_ + visible < cat.count)
      arrow(fb, W - 9, listTop + visible * rowH - 1, false, RGB(120, 120, 130));

    // --- footer hint
    if (!compact) {
      const char* hint = "<> GROUP   OK START";
      int hw = font3x5::textWidth(hint, 1);
      font3x5::drawText(fb, hint, (W - hw) / 2, H - 7, 1, RGB(85, 85, 92));
    }
  }

 private:
  static const int ICON = 11;

  struct Item {
    char label[16];
    int scene;
  };
  struct Category {
    int count = 0;
    Item items[MAX_ITEMS];
  };

  Category cats_[CAT_COUNT];
  int cat_ = 0;
  int item_ = 0;
  int scroll_ = 0;
  bool active_ = false;
  float pillY_ = -1.0f;
  float tabX_ = -1.0f;

  void add(int c, const char* label, int scene) {
    Category& cat = cats_[c];
    if (cat.count >= MAX_ITEMS) return;
    strncpy(cat.items[cat.count].label, label, sizeof(cat.items[0].label) - 1);
    cat.items[cat.count].label[sizeof(cat.items[0].label) - 1] = 0;
    cat.items[cat.count].scene = scene;
    cat.count++;
  }

  // a filled rectangle with its four corner pixels knocked out
  static void roundRect(FrameBuffer& fb, int x, int y, int w, int h,
                        const RGB& c) {
    fb.rect(x + 1, y, w - 2, h, c);
    fb.rect(x, y + 1, 1, h - 2, c);
    fb.rect(x + w - 1, y + 1, 1, h - 2, c);
  }

  static void arrow(FrameBuffer& fb, int x, int y, bool up, const RGB& c) {
    for (int i = 0; i < 3; i++) {
      int row = up ? y + i : y + 2 - i;
      fb.hLine(x + 2 - i, x + 2 + i, row, c);
    }
  }

  // 11x11 category icons: clock, heart, gamepad, sparkle
  static void drawIcon(FrameBuffer& fb, int cat, int x, int y, const RGB& c) {
    static const uint16_t icons[CAT_COUNT][ICON] = {
        {0x0F8, 0x306, 0x222, 0x421, 0x421, 0x439, 0x401, 0x401, 0x202, 0x306,
         0x0F8},
        {0x000, 0x38E, 0x7DF, 0x7FF, 0x7FF, 0x3FE, 0x1FC, 0x0F8, 0x070, 0x020,
         0x000},
        {0x000, 0x000, 0x3FE, 0x6FB, 0x475, 0x6FB, 0x7FF, 0x707, 0x202, 0x000,
         0x000},
        {0x020, 0x020, 0x070, 0x070, 0x1FC, 0x7FF, 0x1FC, 0x070, 0x070, 0x020,
         0x020},
    };
    for (int r = 0; r < ICON; r++)
      for (int b = 0; b < ICON; b++)
        if (icons[cat][r] & (1 << (ICON - 1 - b))) fb.set(x + b, y + r, c);
  }
};

}

#endif
