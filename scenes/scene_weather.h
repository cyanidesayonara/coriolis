// Weather — an animated sky for the current conditions plus a big
// temperature. Sun with turning rays, drifting clouds, falling rain,
// snow, or a storm with lightning. Without a data source it waits
// patiently (the display never touches the internet itself).
#ifndef CORIOLIS_SCENE_WEATHER_H
#define CORIOLIS_SCENE_WEATHER_H

#include <stdio.h>
#include <math.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"
#include "../core/weather.h"

namespace coriolis {

class WeatherScene : public Scene {
 public:
  explicit WeatherScene(WeatherProvider& provider) : provider_(provider) {}

  const char* name() const { return "Weather"; }

  void start(Context&) {
    for (int i = 0; i < NP; i++) drops_[i].life = 0;
    flashMs_ = 0;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();
    WeatherInfo w = provider_.now();

    if (!w.valid) {
      const char* t = "WEATHER";
      int tw = font3x5::textWidth(t, 2);
      font3x5::drawText(ctx.fb, t, (ctx.fb.width() - tw) / 2,
                        ctx.fb.height() / 2 - 12, 2, RGB(150, 150, 150));
      const char* h = "WAITING FOR DATA";
      int hw = font3x5::textWidth(h, 1);
      font3x5::drawText(ctx.fb, h, (ctx.fb.width() - hw) / 2,
                        ctx.fb.height() / 2 + 6, 1, RGB(90, 90, 90));
      return 250;
    }

    switch (w.condition) {
      case Wx::Clear: drawSun(ctx); break;
      case Wx::Clouds: drawClouds(ctx, 2); break;
      case Wx::Rain: drawClouds(ctx, 1); drawRain(ctx, false); break;
      case Wx::Snow: drawClouds(ctx, 1); drawSnow(ctx); break;
      case Wx::Storm: drawClouds(ctx, 2); drawRain(ctx, true); break;
      default: break;
    }

    // the temperature, big, lower half ('*' renders as the degree sign)
    char t[8];
    snprintf(t, sizeof(t), "%d*", w.tempC);
    int tw = font3x5::textWidth(t, 4);
    font3x5::drawText(ctx.fb, t, (ctx.fb.width() - tw) / 2,
                      ctx.fb.height() - 34, 4, RGB(230, 230, 230));

    return 40;
  }

 private:
  static const int NP = 40;
  struct Drop { float x, y, v; int life; };

  WeatherProvider& provider_;
  Drop drops_[NP];
  uint32_t flashMs_ = 0;

  void drawSun(Context& ctx) {
    int cx = ctx.fb.width() / 2, cy = 34, r = 13;
    RGB core(255, 210, 60), ray(255, 170, 30);
    ctx.fb.fillCircle(cx, cy, r, core);
    float base = ctx.nowMs * 0.0006f;
    for (int i = 0; i < 8; i++) {
      float a = base + i * (TWO_PI_F / 8.0f);
      int x0 = cx + int(cosf(a) * (r + 3)), y0 = cy + int(sinf(a) * (r + 3));
      int x1 = cx + int(cosf(a) * (r + 8)), y1 = cy + int(sinf(a) * (r + 8));
      ctx.fb.thickLine(x0, y0, x1, y1, ray);
    }
  }

  void drawCloud(Context& ctx, int cx, int cy, const RGB& c) {
    ctx.fb.fillCircle(cx - 9, cy + 2, 6, c);
    ctx.fb.fillCircle(cx, cy - 2, 8, c);
    ctx.fb.fillCircle(cx + 10, cy + 2, 6, c);
    ctx.fb.rect(cx - 12, cy + 3, 25, 5, c);
  }

  void drawClouds(Context& ctx, int n) {
    int W = ctx.fb.width();
    // clouds drift and wrap
    int d1 = int(ctx.nowMs / 90) % (W + 60) - 30;
    drawCloud(ctx, d1, 30, RGB(170, 170, 180));
    if (n > 1) {
      int d2 = int(ctx.nowMs / 140 + 70) % (W + 60) - 30;
      drawCloud(ctx, d2, 46, RGB(110, 110, 122));
    }
  }

  void drawRain(Context& ctx, bool storm) {
    // spawn
    for (int i = 0; i < NP; i++) {
      if (drops_[i].life <= 0 && random8() < 60) {
        drops_[i].x = float(randomInt(ctx.fb.width()));
        drops_[i].y = float(38 + randomInt(8));
        drops_[i].v = 1.4f + random8(0, 100) / 90.0f;
        drops_[i].life = 60;
      }
    }
    RGB blue(90, 140, 230);
    for (int i = 0; i < NP; i++) {
      Drop& d = drops_[i];
      if (d.life <= 0) continue;
      d.y += d.v;
      d.life--;
      if (d.y > ctx.fb.height() - 36) { d.life = 0; continue; }
      ctx.fb.line(int(d.x), int(d.y), int(d.x), int(d.y) + 2, blue);
    }
    if (storm) {
      // occasional lightning: a jagged bolt plus a brief sky flash
      if (ctx.nowMs - flashMs_ > 2200 && random8() < 6) flashMs_ = ctx.nowMs;
      if (ctx.nowMs - flashMs_ < 120) {
        int x = ctx.fb.width() / 2 + int(random8(0, 30)) - 15;
        RGB bolt(255, 255, 160);
        ctx.fb.thickLine(x, 34, x - 5, 52, bolt);
        ctx.fb.thickLine(x - 5, 52, x + 3, 68, bolt);
        for (int yy = 0; yy < 30; yy++)
          for (int xx = 0; xx < ctx.fb.width(); xx++)
            ctx.fb.at(xx, yy).add(RGB(30, 30, 40));
      }
    }
  }

  void drawSnow(Context& ctx) {
    for (int i = 0; i < NP; i++) {
      if (drops_[i].life <= 0 && random8() < 30) {
        drops_[i].x = float(randomInt(ctx.fb.width()));
        drops_[i].y = float(38 + randomInt(8));
        drops_[i].v = 0.3f + random8(0, 100) / 300.0f;
        drops_[i].life = 250;
      }
    }
    RGB white(230, 235, 245);
    for (int i = 0; i < NP; i++) {
      Drop& d = drops_[i];
      if (d.life <= 0) continue;
      d.y += d.v;
      d.x += sinf(d.y * 0.25f) * 0.4f;
      d.life--;
      if (d.y > ctx.fb.height() - 36) { d.life = 0; continue; }
      ctx.fb.set(int(d.x), int(d.y), white);
    }
  }
};

}

#endif
