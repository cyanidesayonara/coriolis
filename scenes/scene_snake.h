// Snake — the classic. Arrows steer, the snake grows on food, and hitting
// a wall or itself ends the run. A game, so it uses fixed colors (never
// themed) and opens on a setup card. Speed ramps a little as you grow.
#ifndef CORIOLIS_SCENE_SNAKE_H
#define CORIOLIS_SCENE_SNAKE_H

#include <stdio.h>
#include <string.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"
#include "../core/settings.h"
#include "intro.h"

namespace coriolis {

class SnakeScene : public Scene {
 public:
  explicit SnakeScene(Settings& settings) : settings_(settings) {}

  const char* name() const { return "Snake"; }
  bool autoplayEligible() const { return false; }

  void start(Context&) { started_ = false; }

  void begin(Context& ctx) {
    gw_ = ctx.fb.width() / CELL;
    gh_ = ctx.fb.height() / CELL;
    if (gw_ > MAXG) gw_ = MAXG;
    if (gh_ > MAXG) gh_ = MAXG;

    memset(occ_, 0, sizeof(occ_));
    len_ = 3;
    dir_ = 1;  // moving right
    pendingDir_ = 1;
    dead_ = false;
    score_ = 0;

    int cx = gw_ / 2, cy = gh_ / 2;
    for (int i = 0; i < len_; i++) {
      body_[i].x = cx - i;
      body_[i].y = cy;
      occ_[body_[i].x][body_[i].y] = true;
    }
    head_ = 0;  // body_[head_] is the head; older cells follow in the ring
    lastStepMs_ = ctx.nowMs;
    placeFood();
    started_ = true;
    ctx.audio.play(Cue::StartBell);
  }

  bool input(Context& ctx, Key k) {
    if (!started_) {
      if (k == Key::Select) { begin(ctx); return true; }
      if (k == Key::Up || k == Key::Down) {
        int s = settings_.snakeSpeed + (k == Key::Up ? 1 : -1);
        settings_.snakeSpeed = uint8_t(s < 0 ? 0 : (s > 2 ? 2 : s));
        return true;
      }
      return false;
    }
    if (dead_) {
      if (k == Key::Select) { begin(ctx); return true; }
      return false;
    }
    // 0 up, 1 right, 2 down, 3 left — can't reverse straight back
    int nd = -1;
    if (k == Key::Up) nd = 0;
    else if (k == Key::Right) nd = 1;
    else if (k == Key::Down) nd = 2;
    else if (k == Key::Left) nd = 3;
    if (nd >= 0 && (nd + 2) % 4 != dir_) {
      pendingDir_ = nd;
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    if (!started_) {
      static const char* sp[3] = {"SLOW", "NORMAL", "FAST"};
      const char* lines[] = {sp[settings_.snakeSpeed], "ARROWS TO TURN"};
      intro::draw(ctx, "SNAKE", lines, 2, RGB(90, 230, 90));
      return 60;
    }

    uint32_t step = stepMs();
    if (!dead_ && ctx.nowMs - lastStepMs_ >= step) {
      lastStepMs_ = ctx.nowMs;
      advance(ctx);
    }

    render(ctx);
    return 20;
  }

 private:
  struct Cell { int8_t x, y; };
  static const int CELL = 4;    // pixels per grid cell
  static const int MAXG = 40;   // max grid dimension
  static const int MAXLEN = MAXG * MAXG;

  Settings& settings_;
  bool started_ = false;

  Cell body_[MAXLEN];
  bool occ_[MAXG][MAXG];
  int head_, len_;
  int gw_, gh_;
  int dir_, pendingDir_;
  Cell food_;
  bool dead_;
  int score_;
  uint32_t lastStepMs_ = 0;

  uint32_t stepMs() const {
    // base by setting, quickening slightly as the snake grows
    int base = settings_.snakeSpeed == 0 ? 150
                                         : (settings_.snakeSpeed == 1 ? 110 : 75);
    int fast = base - len_;
    return uint32_t(fast < base / 2 ? base / 2 : fast);
  }

  Cell& at(int i) { return body_[(head_ + i) % MAXLEN]; }  // i=0 head

  void placeFood() {
    if (len_ >= gw_ * gh_) return;
    for (int tries = 0; tries < 400; tries++) {
      int x = randomInt(gw_), y = randomInt(gh_);
      if (!occ_[x][y]) { food_.x = int8_t(x); food_.y = int8_t(y); return; }
    }
  }

  void advance(Context& ctx) {
    dir_ = pendingDir_;
    Cell h = at(0);
    int nx = h.x + (dir_ == 1) - (dir_ == 3);
    int ny = h.y + (dir_ == 2) - (dir_ == 0);

    if (nx < 0 || ny < 0 || nx >= gw_ || ny >= gh_) {
      dead_ = true;
      ctx.audio.play(Cue::Die);
      return;
    }

    Cell tail = at(len_ - 1);
    bool growing = (nx == food_.x && ny == food_.y);
    // hitting the body kills, except the tail cell which is about to vacate
    if (occ_[nx][ny] && !(nx == tail.x && ny == tail.y && !growing)) {
      dead_ = true;
      ctx.audio.play(Cue::Die);
      return;
    }

    if (!growing) occ_[tail.x][tail.y] = false;

    head_ = (head_ + MAXLEN - 1) % MAXLEN;  // new head slot in front
    body_[head_].x = int8_t(nx);
    body_[head_].y = int8_t(ny);
    occ_[nx][ny] = true;

    if (growing) {
      if (len_ < MAXLEN) len_++;
      score_++;
      ctx.audio.play(Cue::Eat);
      placeFood();
    }
  }

  void render(Context& ctx) {
    ctx.fb.clear();

    // playfield border
    RGB border(40, 40, 40);
    ctx.fb.rect(0, 0, gw_ * CELL, 1, border);
    ctx.fb.rect(0, gh_ * CELL - 1, gw_ * CELL, 1, border);
    ctx.fb.rect(0, 0, 1, gh_ * CELL, border);
    ctx.fb.rect(gw_ * CELL - 1, 0, 1, gh_ * CELL, border);

    // food
    ctx.fb.rect(food_.x * CELL + 1, food_.y * CELL + 1, CELL - 2, CELL - 2,
                RGB(230, 40, 40));

    // snake: bright head, body fading back a touch
    for (int i = 0; i < len_; i++) {
      Cell s = at(i);
      RGB c = i == 0 ? RGB(180, 255, 180) : RGB(40, 200, 60);
      ctx.fb.rect(s.x * CELL, s.y * CELL, CELL - 1, CELL - 1, c);
    }

    char scoreText[8];
    snprintf(scoreText, sizeof(scoreText), "%d", score_);
    font3x5::drawText(ctx.fb, scoreText, 3, 3, 1, RGB(150, 150, 150));

    if (dead_) {
      const char* go = "GAME OVER";
      int w = font3x5::textWidth(go, 2);
      font3x5::drawText(ctx.fb, go, (ctx.fb.width() - w) / 2,
                        ctx.fb.height() / 2 - 8, 2, RGB(230, 80, 80));
      const char* again = "OK TO RETRY";
      int aw = font3x5::textWidth(again, 1);
      font3x5::drawText(ctx.fb, again, (ctx.fb.width() - aw) / 2,
                        ctx.fb.height() / 2 + 6, 1, RGB(150, 150, 150));
    }
  }
};

}

#endif
