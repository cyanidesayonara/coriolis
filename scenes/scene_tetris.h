// Tetris — the portrait classic. A 10x20 well fits naturally in the tall
// half of the square display, with a NEXT preview and score alongside.
// Arrows move/rotate, Down soft-drops, OK hard-drops. A game: fixed piece
// colors, an intro card, retry on top-out.
#ifndef CORIOLIS_SCENE_TETRIS_H
#define CORIOLIS_SCENE_TETRIS_H

#include <stdio.h>
#include <string.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"
#include "intro.h"

namespace coriolis {

// 7 tetrominoes x 4 rotations x 4 cells x (col,row) in a 4x4 local box
static const int8_t TETROMINO[7][4][4][2] = {
  { {{0,1},{1,1},{2,1},{3,1}}, {{2,0},{2,1},{2,2},{2,3}},   // I
    {{0,2},{1,2},{2,2},{3,2}}, {{1,0},{1,1},{1,2},{1,3}} },
  { {{0,0},{0,1},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{1,2}},   // J
    {{0,1},{1,1},{2,1},{2,2}}, {{1,0},{1,1},{0,2},{1,2}} },
  { {{2,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{2,2}},   // L
    {{0,1},{1,1},{2,1},{0,2}}, {{0,0},{1,0},{1,1},{1,2}} },
  { {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}},   // O
    {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}} },
  { {{1,0},{2,0},{0,1},{1,1}}, {{1,0},{1,1},{2,1},{2,2}},   // S
    {{1,1},{2,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{1,2}} },
  { {{1,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{2,1},{1,2}},   // T
    {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}} },
  { {{0,0},{1,0},{1,1},{2,1}}, {{2,0},{1,1},{2,1},{1,2}},   // Z
    {{0,1},{1,1},{1,2},{2,2}}, {{1,0},{0,1},{1,1},{0,2}} },
};

class TetrisScene : public Scene {
 public:
  const char* name() const { return "Tetris"; }
  bool autoplayEligible() const { return false; }

  void start(Context&) { started_ = false; }

  void begin(Context& ctx) {
    memset(grid_, 0, sizeof(grid_));
    score_ = lines_ = 0;
    dead_ = false;
    bagIdx_ = 7;
    next_ = drawBag();
    spawn(ctx);
    lastFall_ = ctx.nowMs;
    started_ = true;
    ctx.audio.play(Cue::StartBell);
  }

  bool input(Context& ctx, Key k) {
    if (!started_) {
      if (k == Key::Select) { begin(ctx); return true; }
      if (k == Key::Up || k == Key::Down) {
        startLevel_ += (k == Key::Up) ? 1 : -1;
        if (startLevel_ < 0) startLevel_ = 0;
        if (startLevel_ > 9) startLevel_ = 9;
        return true;
      }
      return false;
    }
    if (dead_) {
      if (k == Key::Select) { begin(ctx); return true; }
      return false;
    }
    // consume every control key while playing (even a blocked move), so a
    // move into the wall never leaks out and switches scenes
    switch (k) {
      case Key::Left:
        if (fits(type_, rot_, px_ - 1, py_)) px_--;
        return true;
      case Key::Right:
        if (fits(type_, rot_, px_ + 1, py_)) px_++;
        return true;
      case Key::Up:
        tryRotate();
        return true;
      case Key::Down:
        return true;  // soft drop is read from the held state each frame
      case Key::Select:
        hardDrop(ctx);  // OK slams it down
        return true;
      default:
        return false;  // Back falls through, to leave the game
    }
  }

  uint32_t draw(Context& ctx) {
    if (!started_) {
      char l0[16];
      snprintf(l0, sizeof(l0), "START LEVEL %d", startLevel_);
      const char* lines[] = {l0, "ARROWS  OK DROP"};
      intro::draw(ctx, "TETRIS", lines, 2, RGB(90, 210, 230));
      return 60;
    }

    if (!dead_) {
      bool soft = ctx.held.isDown(Key::Down);
      uint32_t interval = soft ? 40 : gravityMs();
      if (ctx.nowMs - lastFall_ >= interval) {
        lastFall_ = ctx.nowMs;
        stepDown(ctx);
      }
    }

    render(ctx);
    return 20;
  }

 private:
  // classic Tetris is 10 wide x 20 tall; bigger cells fill the tall display
  static const int GW = 10, GH = 20, CELL = 6;

  uint8_t grid_[GW][GH];  // 0 empty, else colorIndex 1..7
  int type_, rot_, px_, py_, next_;
  int score_, lines_, startLevel_ = 0;
  bool started_ = false, dead_ = false;
  uint8_t bag_[7];
  int bagIdx_ = 7;
  uint32_t lastFall_ = 0;

  static RGB pieceColor(int t) {
    static const RGB c[7] = {
        {0, 240, 240}, {40, 70, 240}, {240, 140, 0}, {240, 240, 0},
        {0, 220, 60},  {170, 50, 220}, {240, 40, 40}};
    return c[t];
  }

  int drawBag() {
    if (bagIdx_ >= 7) {
      for (int i = 0; i < 7; i++) bag_[i] = uint8_t(i);
      for (int i = 6; i > 0; i--) {  // Fisher-Yates
        int j = randomInt(i + 1);
        uint8_t t = bag_[i]; bag_[i] = bag_[j]; bag_[j] = t;
      }
      bagIdx_ = 0;
    }
    return bag_[bagIdx_++];
  }

  void spawn(Context& ctx) {
    type_ = next_;
    next_ = drawBag();
    rot_ = 0;
    px_ = 3;
    py_ = 0;
    if (!fits(type_, rot_, px_, py_)) {  // top-out
      dead_ = true;
      ctx.audio.play(Cue::Die);
    }
  }

  bool fits(int t, int r, int ox, int oy) const {
    for (int i = 0; i < 4; i++) {
      int x = ox + TETROMINO[t][r][i][0];
      int y = oy + TETROMINO[t][r][i][1];
      if (x < 0 || x >= GW || y < 0 || y >= GH) return false;
      if (grid_[x][y]) return false;
    }
    return true;
  }

  void tryRotate() {
    int nr = (rot_ + 1) % 4;
    if (fits(type_, nr, px_, py_)) { rot_ = nr; return; }
    if (fits(type_, nr, px_ - 1, py_)) { rot_ = nr; px_--; return; }  // kicks
    if (fits(type_, nr, px_ + 1, py_)) { rot_ = nr; px_++; return; }
  }

  void lockPiece(Context& ctx) {
    for (int i = 0; i < 4; i++) {
      int x = px_ + TETROMINO[type_][rot_][i][0];
      int y = py_ + TETROMINO[type_][rot_][i][1];
      if (x >= 0 && x < GW && y >= 0 && y < GH) grid_[x][y] = uint8_t(type_ + 1);
    }
    clearLines(ctx);
    ctx.audio.play(Cue::Chime);
    spawn(ctx);
  }

  void stepDown(Context& ctx) {
    if (fits(type_, rot_, px_, py_ + 1)) py_++;
    else lockPiece(ctx);
  }

  void hardDrop(Context& ctx) {
    while (fits(type_, rot_, px_, py_ + 1)) py_++;
    lockPiece(ctx);
    lastFall_ = ctx.nowMs;
  }

  void clearLines(Context& ctx) {
    int cleared = 0;
    for (int y = GH - 1; y >= 0; y--) {
      bool full = true;
      for (int x = 0; x < GW; x++) if (!grid_[x][y]) { full = false; break; }
      if (!full) continue;
      cleared++;
      for (int yy = y; yy > 0; yy--)          // drop everything above
        for (int x = 0; x < GW; x++) grid_[x][yy] = grid_[x][yy - 1];
      for (int x = 0; x < GW; x++) grid_[x][0] = 0;
      y++;  // recheck this row
    }
    if (cleared) {
      static const int pts[5] = {0, 100, 300, 500, 800};
      score_ += pts[cleared] * (level() + 1);
      lines_ += cleared;
      ctx.audio.play(Cue::Score);
    }
  }

  int level() const { return startLevel_ + lines_ / 10; }
  uint32_t gravityMs() const {
    int g = 800 - level() * 70;
    return uint32_t(g < 80 ? 80 : g);
  }

  void cell(Context& ctx, int gx, int gy, const RGB& c, int ox, int oy) {
    ctx.fb.rect(ox + gx * CELL, oy + gy * CELL, CELL - 1, CELL - 1, c);
  }

  void render(Context& ctx) {
    ctx.fb.clear();
    int bx = 6, by = 4;  // board fills the left; panel on the right

    // well border
    RGB border(60, 60, 60);
    ctx.fb.rect(bx - 2, by - 2, GW * CELL + 4, 2, border);
    ctx.fb.rect(bx - 2, by + GH * CELL, GW * CELL + 4, 2, border);
    ctx.fb.rect(bx - 2, by - 2, 2, GH * CELL + 4, border);
    ctx.fb.rect(bx + GW * CELL, by - 2, 2, GH * CELL + 4, border);

    // settled blocks
    for (int y = 0; y < GH; y++)
      for (int x = 0; x < GW; x++)
        if (grid_[x][y]) cell(ctx, x, y, pieceColor(grid_[x][y] - 1), bx, by);

    // falling piece + its landing shadow
    if (!dead_) {
      int sy = py_;
      while (fits(type_, rot_, px_, sy + 1)) sy++;
      RGB ghost = pieceColor(type_);
      ghost.dim(60);
      for (int i = 0; i < 4; i++)
        cell(ctx, px_ + TETROMINO[type_][rot_][i][0],
             sy + TETROMINO[type_][rot_][i][1], ghost, bx, by);
      for (int i = 0; i < 4; i++)
        cell(ctx, px_ + TETROMINO[type_][rot_][i][0],
             py_ + TETROMINO[type_][rot_][i][1], pieceColor(type_), bx, by);
    }

    // side panel: NEXT preview, score, lines
    int panelX = bx + GW * CELL + 6;
    RGB label(150, 150, 150), value(230, 230, 230);
    font3x5::drawText(ctx.fb, "NEXT", panelX, by + 4, 1, label);
    for (int i = 0; i < 4; i++)
      cell(ctx, TETROMINO[next_][0][i][0], TETROMINO[next_][0][i][1],
           pieceColor(next_), panelX, by + 14);

    char buf[12];
    snprintf(buf, sizeof(buf), "%d", score_);
    font3x5::drawText(ctx.fb, "SCORE", panelX, by + 46, 1, label);
    font3x5::drawText(ctx.fb, buf, panelX, by + 55, 1, value);
    snprintf(buf, sizeof(buf), "%d", lines_);
    font3x5::drawText(ctx.fb, "LINES", panelX, by + 74, 1, label);
    font3x5::drawText(ctx.fb, buf, panelX, by + 83, 1, value);
    snprintf(buf, sizeof(buf), "%d", level());
    font3x5::drawText(ctx.fb, "LEVEL", panelX, by + 102, 1, label);
    font3x5::drawText(ctx.fb, buf, panelX, by + 111, 1, value);

    if (dead_) {
      const char* go = "GAME";
      const char* ov = "OVER";
      font3x5::drawText(ctx.fb, go, bx + 6, by + GH * CELL / 2 - 12, 2,
                        RGB(230, 80, 80));
      font3x5::drawText(ctx.fb, ov, bx + 6, by + GH * CELL / 2 + 2, 2,
                        RGB(230, 80, 80));
    }
  }
};

}

#endif
