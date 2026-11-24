// Pong — the first game scene. Starts as an ambient AI-vs-AI match; the
// moment you touch up/down you take over the left paddle. First to 9 wins
// and the match resets. (The PongClock variant, where the score is the
// time, comes later and will reuse this machinery.)
#ifndef CORIOLIS_SCENE_PONG_H
#define CORIOLIS_SCENE_PONG_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/font5x7.h"
#include "../core/math8.h"
#include "../core/settings.h"
#include "intro.h"

namespace coriolis {

class PongScene : public Scene {
 public:
  explicit PongScene(Settings& settings) : settings_(settings) {}

  const char* name() const { return "Pong"; }

  // a game is never part of the ambient autoplay rotation
  bool autoplayEligible() const { return false; }

  void start(Context& ctx) { started_ = false; }

  void begin(Context& ctx) {
    scoreL_ = scoreR_ = 0;
    leftY_ = rightY_ = ctx.fb.height() / 2.0f;
    playerLeft_ = false;
    paused_ = false;
    pauseRow_ = 0;
    lastMs_ = ctx.nowMs;
    serveDir_ = 1;
    started_ = true;
    serve(ctx, ctx.nowMs + 800);
  }

  bool input(Context& ctx, Key k) {
    if (!started_) {  // setup screen: OK starts, up/down set difficulty
      if (k == Key::Select) { begin(ctx); return true; }
      if (k == Key::Up || k == Key::Down) {
        int l = settings_.pongLevel + (k == Key::Up ? 1 : -1);
        settings_.pongLevel = uint8_t(l < 0 ? 0 : (l > 2 ? 2 : l));
        return true;
      }
      return false;
    }
    if (paused_) {
      switch (k) {
        case Key::Up: pauseRow_ = (pauseRow_ + 2) % 3; return true;
        case Key::Down: pauseRow_ = (pauseRow_ + 1) % 3; return true;
        case Key::Left:
        case Key::Right: return true;
        case Key::Back: paused_ = false; lastMs_ = ctx.nowMs; return true;
        case Key::Select:
          if (pauseRow_ == 0) { paused_ = false; lastMs_ = ctx.nowMs; }
          else if (pauseRow_ == 1) { begin(ctx); }
          else { paused_ = false; started_ = false; }
          return true;
        default: return false;
      }
    }
    if (k == Key::Back) { started_ = false; return true; }
    if (k == Key::Up || k == Key::Down) {
      playerLeft_ = true;  // movement itself reads the held state each frame
      return true;
    }
    if (k == Key::Left || k == Key::Right)
      return true;  // unused here, but consume so it can't switch scenes
    if (k == Key::Select) {  // open compact pause/exit menu
      paused_ = true;
      pauseRow_ = 0;
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    if (!started_) {
      static const char* levels[3] = {"EASY", "NORMAL", "HARD"};
      char l0[16];
      snprintf(l0, sizeof(l0), "%s", levels[settings_.pongLevel]);
      const char* lines[] = {l0, "FIRST TO 9"};
      intro::draw(ctx, "PONG", lines, 2, RGB(230, 230, 230));
      return 60;
    }

    const int w = ctx.fb.width();
    const int h = ctx.fb.height();
    const float paddleHalf = h / 12.0f;

    // clamped dt keeps physics sane across scene switches and stalls
    float dt = (ctx.nowMs - lastMs_) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;
    lastMs_ = ctx.nowMs;
    if (paused_) dt = 0.0f;

    // release a finished match: reset scores when its long serve pause ends
    if (!paused_ && (scoreL_ >= WIN_SCORE || scoreR_ >= WIN_SCORE) &&
        ctx.nowMs >= serveAtMs_) {
      scoreL_ = scoreR_ = 0;
    }

    // --- update -------------------------------------------------------
    if (!paused_) {
      if (playerLeft_) {
        if (ctx.held.isDown(Key::Up)) leftY_ -= PADDLE_SPEED * dt;
        if (ctx.held.isDown(Key::Down)) leftY_ += PADDLE_SPEED * dt;
      } else {
        steerAi(leftY_, ballVX_ < 0, dt, h);
      }
      steerAi(rightY_, ballVX_ > 0, dt, h);

      clampPaddle(leftY_, paddleHalf, h);
      clampPaddle(rightY_, paddleHalf, h);
    }

    bool serving = ctx.nowMs < serveAtMs_;
    if (!paused_ && !serving) {
      for (int i = TRAIL - 1; i > 0; i--) {
        trailX_[i] = trailX_[i - 1];
        trailY_[i] = trailY_[i - 1];
      }
      trailX_[0] = ballX_;
      trailY_[0] = ballY_;
      if (trailLen_ < TRAIL) trailLen_++;

      ballX_ += ballVX_ * dt;
      ballY_ += ballVY_ * dt;

      // top/bottom walls
      if (ballY_ < 1) { ballY_ = 1; ballVY_ = -ballVY_; }
      if (ballY_ > h - 2) { ballY_ = float(h - 2); ballVY_ = -ballVY_; }

      // top/bottom walls also click
      if (ballY_ <= 1 || ballY_ >= h - 2) ctx.audio.play(Cue::Bounce);

      // paddles: reflect, speed up a little, hit position sets the angle
      if (ballVX_ < 0 && ballX_ <= PADDLE_X + 2 && ballX_ >= PADDLE_X - 1 &&
          hits(leftY_, paddleHalf)) {
        bounce(leftY_, paddleHalf);
        ballX_ = PADDLE_X + 2;
        ctx.audio.play(Cue::Bounce);
      }
      if (ballVX_ > 0 && ballX_ >= w - PADDLE_X - 3 &&
          ballX_ <= w - PADDLE_X && hits(rightY_, paddleHalf)) {
        bounce(rightY_, paddleHalf);
        ballX_ = float(w - PADDLE_X - 3);
        ctx.audio.play(Cue::Bounce);
      }

      // out past an edge: point scored
      if (ballX_ < -2) {
        scoreR_++;
        ctx.audio.play(Cue::Score);
        endPoint(ctx);
      } else if (ballX_ > w + 2) {
        scoreL_++;
        ctx.audio.play(Cue::Score);
        endPoint(ctx);
      }
    }

    // --- render: fixed classic colors — games are never themed, a palette
    // must not be able to make the paddles invisible ---------------------
    ctx.fb.clear();

    RGB fieldColor(60, 60, 60);
    for (int y = 0; y < h; y += 4) ctx.fb.vLine(w / 2, y, y + 1, fieldColor);

    // scores blink while a finished match waits to reset
    bool matchOver = scoreL_ >= WIN_SCORE || scoreR_ >= WIN_SCORE;
    bool showScores = !matchOver || (ctx.nowMs / 250) % 2 == 0;
    if (showScores) {
      RGB scoreColor(165, 165, 175);
      int sc = h >= 100 ? 2 : 1;
      char text[2] = {char('0' + scoreL_), 0};
      int sw = font5x7::textWidth(text, sc);
      font5x7::drawText(ctx.fb, text, w / 2 - sw - 8, 4, sc, scoreColor);
      text[0] = char('0' + scoreR_);
      font5x7::drawText(ctx.fb, text, w / 2 + 8, 4, sc, scoreColor);
    }

    RGB paddleColor(235, 235, 240);
    ctx.fb.rect(PADDLE_X, int(leftY_ - paddleHalf), 2, int(paddleHalf * 2),
                paddleColor);
    ctx.fb.rect(w - PADDLE_X - 2, int(rightY_ - paddleHalf), 2,
                int(paddleHalf * 2), paddleColor);

    if (!serving || (ctx.nowMs / 150) % 2 == 0) {
      for (int i = trailLen_ - 1; i >= 0; i--) {
        uint8_t br = uint8_t(110 - i * 25);
        ctx.fb.rect(int(trailX_[i]) - 1, int(trailY_[i]) - 1, 2, 2,
                    RGB(br, br, br));
      }
      ctx.fb.rect(int(ballX_) - 1, int(ballY_) - 1, 3, 3,
                  RGB(255, 255, 255));
    }

    if (paused_) {
      int bw = 74, bh = h >= 100 ? 46 : 40;
      int bx = (w - bw) / 2, by = (h - bh) / 2;
      for (int y = by; y < by + bh; y++)
        for (int x = bx; x < bx + bw; x++) ctx.fb.at(x, y).dim(40);
      RGB edge(180, 180, 195);
      ctx.fb.hLine(bx, bx + bw - 1, by, edge);
      ctx.fb.hLine(bx, bx + bw - 1, by + bh - 1, edge);
      ctx.fb.vLine(bx, by, by + bh - 1, edge);
      ctx.fb.vLine(bx + bw - 1, by, by + bh - 1, edge);
      font5x7::drawCentered(ctx.fb, "Paused", w / 2, by + 3, 1, RGB(240, 240, 245));
      static const char* opts[3] = {"Resume", "Restart", "Exit"};
      int rowH = h >= 100 ? 11 : 9;
      int y0 = by + (h >= 100 ? 14 : 12);
      for (int i = 0; i < 3; i++) {
        int ry = y0 + i * rowH;
        if (i == pauseRow_) ctx.fb.rect(bx + 4, ry - 1, bw - 8, rowH - 1, RGB(70, 70, 90));
        font5x7::drawCentered(ctx.fb, opts[i], w / 2, ry, 1,
                              i == pauseRow_ ? RGB(255, 255, 255) : RGB(150, 150, 160));
      }
    }

    return 16;
  }

 private:
  static const int PADDLE_X = 3;
  static const int WIN_SCORE = 9;
  static const int PADDLE_SPEED = 80;  // px/s, player
  static const int TRAIL = 4;

  Settings& settings_;

  // px/s by difficulty setting; normal stays beatable on purpose
  int aiSpeed() const {
    return settings_.pongLevel == 0 ? 48 : (settings_.pongLevel == 1 ? 62 : 78);
  }

  float ballX_, ballY_, ballVX_, ballVY_;
  float trailX_[TRAIL], trailY_[TRAIL];
  int trailLen_ = 0;
  float leftY_, rightY_;
  int scoreL_, scoreR_;
  bool playerLeft_;
  bool started_ = false;
  bool paused_ = false;
  int pauseRow_ = 0;
  int serveDir_;
  uint32_t lastMs_, serveAtMs_;

  void serve(Context& ctx, uint32_t atMs) {
    ballX_ = ctx.fb.width() / 2.0f;
    ballY_ = ctx.fb.height() / 2.0f;
    ballVX_ = 55.0f * serveDir_;
    ballVY_ = (random8() % 2 ? 1 : -1) * (20.0f + random8(30));
    trailLen_ = 0;
    serveAtMs_ = atMs;
  }

  void endPoint(Context& ctx) {
    serveDir_ = -serveDir_;
    // a finished match blinks its final score during a longer serve pause;
    // draw() resets the scores when that serve releases
    bool matchOver = scoreL_ >= WIN_SCORE || scoreR_ >= WIN_SCORE;
    serve(ctx, ctx.nowMs + (matchOver ? 2500 : 800));
  }

  bool hits(float paddleY, float half) const {
    return ballY_ >= paddleY - half - 1 && ballY_ <= paddleY + half + 1;
  }

  void bounce(float paddleY, float half) {
    ballVX_ = -ballVX_ * 1.06f;
    if (ballVX_ > 140) ballVX_ = 140;
    if (ballVX_ < -140) ballVX_ = -140;
    // angle from where the ball met the paddle: center = flat, edge = steep
    ballVY_ = (ballY_ - paddleY) / half * 70.0f;
  }

  void steerAi(float& paddleY, bool ballIncoming, float dt, int h) {
    float target = ballIncoming ? ballY_ : h / 2.0f;
    float diff = target - paddleY;
    if (diff > 2) paddleY += aiSpeed() * dt;
    else if (diff < -2) paddleY -= aiSpeed() * dt;
  }

  static void clampPaddle(float& y, float half, int h) {
    if (y < half) y = half;
    if (y > h - half) y = float(h) - half;
  }
};

}

#endif
