// Pong — the first game scene. Starts as an ambient AI-vs-AI match; the
// moment you touch up/down you take over the left paddle. First to 9 wins
// and the match resets. (The PongClock variant, where the score is the
// time, comes later and will reuse this machinery.)
#ifndef CORIOLIS_SCENE_PONG_H
#define CORIOLIS_SCENE_PONG_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"
#include "../core/settings.h"

namespace coriolis {

class PongScene : public Scene {
 public:
  explicit PongScene(Settings& settings) : settings_(settings) {}

  const char* name() const { return "Pong"; }

  // fine to autoplay past the ambient AI match, never past a human game
  bool autoplayEligible() const { return !playerLeft_; }

  void start(Context& ctx) {
    scoreL_ = scoreR_ = 0;
    leftY_ = rightY_ = ctx.fb.height() / 2.0f;
    playerLeft_ = false;
    lastMs_ = ctx.nowMs;
    serveDir_ = 1;
    serve(ctx, ctx.nowMs + 800);
  }

  bool input(Context&, Key k) {
    if (k == Key::Up || k == Key::Down) {
      playerLeft_ = true;  // movement itself reads the held state each frame
      return true;
    }
    if (k == Key::Select) {  // restart the match
      scoreL_ = scoreR_ = 0;
      playerLeft_ = false;
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    const int w = ctx.fb.width();
    const int h = ctx.fb.height();
    const float paddleHalf = h / 12.0f;

    // clamped dt keeps physics sane across scene switches and stalls
    float dt = (ctx.nowMs - lastMs_) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;
    lastMs_ = ctx.nowMs;

    // release a finished match: reset scores when its long serve pause ends
    if ((scoreL_ >= WIN_SCORE || scoreR_ >= WIN_SCORE) &&
        ctx.nowMs >= serveAtMs_) {
      scoreL_ = scoreR_ = 0;
    }

    // --- update -------------------------------------------------------
    if (playerLeft_) {
      if (ctx.held.isDown(Key::Up)) leftY_ -= PADDLE_SPEED * dt;
      if (ctx.held.isDown(Key::Down)) leftY_ += PADDLE_SPEED * dt;
    } else {
      steerAi(leftY_, ballVX_ < 0, dt, h);
    }
    steerAi(rightY_, ballVX_ > 0, dt, h);

    clampPaddle(leftY_, paddleHalf, h);
    clampPaddle(rightY_, paddleHalf, h);

    bool serving = ctx.nowMs < serveAtMs_;
    if (!serving) {
      ballX_ += ballVX_ * dt;
      ballY_ += ballVY_ * dt;

      // top/bottom walls
      if (ballY_ < 1) { ballY_ = 1; ballVY_ = -ballVY_; }
      if (ballY_ > h - 2) { ballY_ = float(h - 2); ballVY_ = -ballVY_; }

      // paddles: reflect, speed up a little, hit position sets the angle
      if (ballVX_ < 0 && ballX_ <= PADDLE_X + 2 && ballX_ >= PADDLE_X - 1 &&
          hits(leftY_, paddleHalf)) {
        bounce(leftY_, paddleHalf);
        ballX_ = PADDLE_X + 2;
      }
      if (ballVX_ > 0 && ballX_ >= w - PADDLE_X - 3 &&
          ballX_ <= w - PADDLE_X && hits(rightY_, paddleHalf)) {
        bounce(rightY_, paddleHalf);
        ballX_ = float(w - PADDLE_X - 3);
      }

      // out past an edge: point scored
      if (ballX_ < -2) {
        scoreR_++;
        endPoint(ctx);
      } else if (ballX_ > w + 2) {
        scoreL_++;
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
      RGB scoreColor(150, 150, 150);
      char text[2] = {char('0' + scoreL_), 0};
      font3x5::drawText(ctx.fb, text, w / 2 - 14, 3, 2, scoreColor);
      text[0] = char('0' + scoreR_);
      font3x5::drawText(ctx.fb, text, w / 2 + 8, 3, 2, scoreColor);
    }

    RGB paddleColor(230, 230, 230);
    ctx.fb.rect(PADDLE_X, int(leftY_ - paddleHalf), 2, int(paddleHalf * 2),
                paddleColor);
    ctx.fb.rect(w - PADDLE_X - 2, int(rightY_ - paddleHalf), 2,
                int(paddleHalf * 2), paddleColor);

    if (!serving || (ctx.nowMs / 150) % 2 == 0) {
      ctx.fb.rect(int(ballX_) - 1, int(ballY_) - 1, 3, 3,
                  RGB(255, 255, 255));
    }

    return 16;
  }

 private:
  static const int PADDLE_X = 3;
  static const int WIN_SCORE = 9;
  static const int PADDLE_SPEED = 80;  // px/s, player

  Settings& settings_;

  // px/s by difficulty setting; normal stays beatable on purpose
  int aiSpeed() const {
    return settings_.pongLevel == 0 ? 48 : (settings_.pongLevel == 1 ? 62 : 78);
  }

  float ballX_, ballY_, ballVX_, ballVY_;
  float leftY_, rightY_;
  int scoreL_, scoreR_;
  bool playerLeft_;
  int serveDir_;
  uint32_t lastMs_, serveAtMs_;

  void serve(Context& ctx, uint32_t atMs) {
    ballX_ = ctx.fb.width() / 2.0f;
    ballY_ = ctx.fb.height() / 2.0f;
    ballVX_ = 55.0f * serveDir_;
    ballVY_ = (random8() % 2 ? 1 : -1) * (20.0f + random8(30));
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
