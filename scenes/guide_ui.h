// Shared look for the guided-activity scenes (yoga, exercise, breathing).
// These scenes keep a fixed, friendly identity — coral figure, teal mat,
// lavender titles — independent of the theme palette. Only the top bar's
// clock is themed, so the display still feels consistent with the rest.
//
// Also the shared pieces of a guided session: the side tag for one-sided
// poses, the "next up" badge, the 3-2-1 get-ready screen and the
// completion card.
#ifndef CORIOLIS_GUIDE_UI_H
#define CORIOLIS_GUIDE_UI_H

#include <stdio.h>
#include <math.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/font5x7.h"
#include "../core/math8.h"

namespace coriolis {
namespace guide {

inline RGB bodyColor() { return RGB(255, 130, 100); }   // warm coral
inline RGB limbColor() { return RGB(235, 90, 120); }    // deeper rose
inline RGB matColor() { return RGB(0, 185, 165); }      // teal
inline RGB titleColor() { return RGB(195, 160, 255); }  // soft lavender
inline RGB mutedColor() { return RGB(110, 110, 110); }
inline RGB workColor() { return RGB(60, 220, 120); }    // interval: go
inline RGB restColor() { return RGB(255, 170, 40); }    // interval: breathe

// top bar: themed clock left, fixed-identity title centered.
// Returns the y where scene content may start.
inline int drawTopBar(Context& ctx, const char* title) {
  TimeOfDay t = ctx.time.now();
  char clock[6];
  snprintf(clock, sizeof(clock), "%02d:%02d", t.hour, t.minute);
  font3x5::drawText(ctx.fb, clock, 2, 2, 1, ctx.palette->lookupBright(0));

  int w = font3x5::textWidth(title, 1);
  font3x5::drawText(ctx.fb, title, (ctx.fb.width() - w) / 2, 2, 1,
                    titleColor());
  return 9;
}

inline void pill(FrameBuffer& fb, int x, int y, int w, int h, const RGB& c) {
  fb.rect(x + 1, y, w - 2, h, c);
  fb.rect(x, y + 1, 1, h - 2, c);
  fb.rect(x + w - 1, y + 1, 1, h - 2, c);
}

// "RIGHT" / "LEFT" tag under the top bar for one-sided poses and moves
inline void drawSideTag(Context& ctx, int side) {
  if (side == 0) return;
  const char* s = side > 0 ? "RIGHT" : "LEFT";
  int w = font3x5::textWidth(s, 1) + 6;
  RGB bg = titleColor();
  bg.dim(70);
  pill(ctx.fb, 2, 11, w, 9, bg);
  font3x5::drawText(ctx.fb, s, 5, 13, 1, RGB(235, 225, 255));
}

// a slim panel above the progress bar announcing what comes next, with the
// seconds left; shown for the last few seconds of a hold
inline void drawNextBadge(Context& ctx, const char* name, int side,
                          int secondsLeft) {
  const int W = ctx.fb.width(), H = ctx.fb.height();
  char line[32];
  snprintf(line, sizeof(line), "NEXT %s%s", name,
           side > 0 ? " R" : (side < 0 ? " L" : ""));
  int tw = font3x5::textWidth(line, 1);
  int bw = tw + 18, bh = 9;
  if (bw > W - 4) bw = W - 4;
  int bx = (W - bw) / 2, by = H - bh - 4;
  for (int y = by; y < by + bh; y++)
    for (int x = bx; x < bx + bw; x++) ctx.fb.at(x, y).dim(40);
  RGB edge = titleColor();
  edge.dim(120);
  ctx.fb.hLine(bx + 1, bx + bw - 2, by, edge);
  font3x5::drawText(ctx.fb, line, bx + 3, by + 2, 1, RGB(220, 220, 228));
  char n[4];
  snprintf(n, sizeof(n), "%d", secondsLeft < 0 ? 0 : secondsLeft % 10);
  font3x5::drawText(ctx.fb, n, bx + bw - 6, by + 2, 1, titleColor());
}

inline void drawPaused(Context& ctx) {
  const int W = ctx.fb.width(), H = ctx.fb.height();
  int bw = 46, bh = 13, bx = (W - bw) / 2, by = H / 2 - 7;
  for (int y = by; y < by + bh; y++)
    for (int x = bx; x < bx + bw; x++) ctx.fb.at(x, y).dim(50);
  ctx.fb.rect(bx + 6, by + 3, 2, 7, titleColor());
  ctx.fb.rect(bx + 10, by + 3, 2, 7, titleColor());
  font5x7::drawText(ctx.fb, "Paused", bx + 15, by + 3, 1, RGB(235, 235, 240));
}

// 3-2-1 get-ready screen: a big rounded numeral inside a ring that drains
// each second, with what's about to start underneath
inline void drawCountdown(Context& ctx, uint32_t elapsedMs, uint32_t totalMs,
                          const char* firstName) {
  ctx.fb.clear();
  const int W = ctx.fb.width(), H = ctx.fb.height();
  int left = int((totalMs - (elapsedMs < totalMs ? elapsedMs : totalMs) + 999) / 1000);
  if (left < 1) left = 1;
  float sec = (elapsedMs % 1000) / 1000.0f;

  int cx = W / 2, cy = H / 2 - 4;
  int r = (W < H ? W : H) / 2 - 18;
  RGB ringDim = titleColor();
  ringDim.dim(45);
  ctx.fb.circle(cx, cy, r, ringDim);
  int lit = int(72 * (1.0f - sec));
  for (int i = 0; i < lit; i++) {
    float a = -1.5708f + i * (TWO_PI_F / 72.0f);
    ctx.fb.set(cx + int(cosf(a) * r), cy + int(sinf(a) * r), titleColor());
    ctx.fb.set(cx + int(cosf(a) * (r - 1)), cy + int(sinf(a) * (r - 1)),
               titleColor());
  }

  char n[3];
  snprintf(n, sizeof(n), "%d", left);
  int scale = H >= 100 ? 5 : 3;
  // each numeral lands big and settles: a small pop at the start of a second
  RGB top(255, 255, 255), bottom = titleColor();
  font5x7::drawCentered(ctx.fb, n, cx, cy - font5x7::GLYPH_H * scale / 2, scale,
                        top, bottom);

  if (H >= 100) {
    font5x7::drawCentered(ctx.fb, "Get ready", W / 2, 6, 1, RGB(170, 170, 178));
    font3x5::drawText(ctx.fb, firstName,
                      (W - font3x5::textWidth(firstName, 1)) / 2, H - 12, 1,
                      titleColor());
  }
}

// end-of-session card: title, stats line, expanding celebration rings
inline void drawCompletion(Context& ctx, const char* title, const char* stats,
                           uint32_t sinceMs) {
  ctx.fb.clear();
  const int W = ctx.fb.width(), H = ctx.fb.height();
  int cx = W / 2, cy = H / 2 - 6;

  // three soft rings ripple outward on a loop
  for (int k = 0; k < 3; k++) {
    uint32_t t = (sinceMs + k * 700) % 2100;
    int r = 8 + int(t * (W / 2) / 2100);
    uint8_t b = uint8_t(160 - t * 160 / 2100);
    RGB c = k == 1 ? matColor() : titleColor();
    c.dim(b);
    ctx.fb.circle(cx, cy, r, c);
  }

  int scale = (font5x7::textWidth(title, 2) <= W - 6 && H >= 100) ? 2 : 1;
  RGB top(255, 240, 210), bottom = bodyColor();
  font5x7::drawCentered(ctx.fb, title, cx, cy - font5x7::GLYPH_H * scale / 2,
                        scale, top, bottom);
  font5x7::drawCentered(ctx.fb, stats, cx, cy + font5x7::GLYPH_H * scale / 2 + 8,
                        1, RGB(190, 190, 198));
  if ((sinceMs / 600) % 2 == 0 || sinceMs < 1200) {
    const char* p = "OK to go again";
    font3x5::drawText(ctx.fb, p, (W - font3x5::textWidth(p, 1)) / 2, H - 9, 1,
                      RGB(95, 95, 105));
  }
}

inline void formatDuration(uint32_t ms, char* out, int n) {
  uint32_t s = ms / 1000;
  snprintf(out, n, "%d:%02d", int(s / 60), int(s % 60));
}

}
}

#endif
