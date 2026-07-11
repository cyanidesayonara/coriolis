// Clock overlay — an optional digital, analog, or word clock drawn on TOP
// of whatever scene is running (a pattern, a GIF, anything). Toggled by its
// own dedicated button; position (3x3 grid) and size live in settings. A
// dimmed panel sits behind it so it stays legible over busy content. This
// is the Borealis "clock over the animation" feature.
#ifndef CORIOLIS_CLOCK_OVERLAY_H
#define CORIOLIS_CLOCK_OVERLAY_H

#include <stdio.h>
#include <math.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"
#include "../core/settings.h"

namespace coriolis {
namespace overlay {

inline void panel(FrameBuffer& fb, int x, int y, int w, int h) {
  for (int yy = y; yy < y + h; yy++)
    for (int xx = x; xx < x + w; xx++)
      if (fb.contains(xx, yy)) fb.at(xx, yy).dim(45);  // ~translucent black
}

inline void anchor(int gridPos, int boxW, int boxH, int W, int H, int& ox,
                   int& oy) {
  int col = gridPos % 3, row = gridPos / 3, m = 3;
  ox = col == 0 ? m : (col == 1 ? (W - boxW) / 2 : W - boxW - m);
  oy = row == 0 ? m : (row == 1 ? (H - boxH) / 2 : H - boxH - m);
}

inline void buildWords(const TimeOfDay& t, const char* lines[3], int& n) {
  static const char* small[20] = {
      "ZERO",    "ONE",     "TWO",       "THREE",    "FOUR",
      "FIVE",    "SIX",     "SEVEN",     "EIGHT",    "NINE",
      "TEN",     "ELEVEN",  "TWELVE",    "THIRTEEN", "FOURTEEN",
      "FIFTEEN", "SIXTEEN", "SEVENTEEN", "EIGHTEEN", "NINETEEN"};
  static const char* tens[4] = {"TWENTY", "THIRTY", "FORTY", "FIFTY"};
  int hour = t.hour % 12;
  if (hour == 0) hour = 12;
  n = 0;
  lines[n++] = small[hour];
  int m = t.minute;
  if (m == 0) {
    lines[n++] = "OCLOCK";
  } else if (m < 10) {
    lines[n++] = "OH";
    lines[n++] = small[m];
  } else if (m < 20) {
    lines[n++] = small[m];
  } else {
    lines[n++] = tens[m / 10 - 2];
    if (m % 10 != 0) lines[n++] = small[m % 10];
  }
}

inline void drawDigital(Context& ctx, int scale, int gridPos) {
  TimeOfDay t = ctx.time.now();
  char txt[6];
  snprintf(txt, sizeof(txt), "%02d:%02d", t.hour, t.minute);
  int tw = font3x5::textWidth(txt, scale), th = font3x5::GLYPH_H * scale;
  int boxW = tw + 4, boxH = th + 4, ox, oy;
  anchor(gridPos, boxW, boxH, ctx.fb.width(), ctx.fb.height(), ox, oy);
  panel(ctx.fb, ox, oy, boxW, boxH);
  font3x5::drawText(ctx.fb, txt, ox + 2, oy + 2, scale, RGB(255, 255, 255));
}

inline void drawAnalog(Context& ctx, int r, int gridPos) {
  int boxW = 2 * r + 5, boxH = 2 * r + 5, ox, oy;
  anchor(gridPos, boxW, boxH, ctx.fb.width(), ctx.fb.height(), ox, oy);
  panel(ctx.fb, ox, oy, boxW, boxH);
  int cx = ox + r + 2, cy = oy + r + 2;
  ctx.fb.circle(cx, cy, r, RGB(210, 210, 210));
  TimeOfDay t = ctx.time.now();
  float ah = ((t.hour % 12) + t.minute / 60.0f) * (TWO_PI_F / 12.0f);
  float am = t.minute * (TWO_PI_F / 60.0f);
  ctx.fb.thickLine(cx, cy, cx + int(sinf(ah) * r * 0.5f),
                   cy - int(cosf(ah) * r * 0.5f), RGB(255, 255, 255));
  ctx.fb.thickLine(cx, cy, cx + int(sinf(am) * r * 0.82f),
                   cy - int(cosf(am) * r * 0.82f), RGB(255, 255, 255));
  ctx.fb.fillCircle(cx, cy, 1, RGB(255, 255, 255));
}

inline void drawWord(Context& ctx, int scale, int gridPos) {
  TimeOfDay t = ctx.time.now();
  const char* lines[3];
  int n = 0;
  buildWords(t, lines, n);
  int maxW = 0;
  for (int i = 0; i < n; i++) {
    int w = font3x5::textWidth(lines[i], scale);
    if (w > maxW) maxW = w;
  }
  int lineH = font3x5::GLYPH_H * scale + scale;
  int boxW = maxW + 4, boxH = n * lineH - scale + 4, ox, oy;
  anchor(gridPos, boxW, boxH, ctx.fb.width(), ctx.fb.height(), ox, oy);
  panel(ctx.fb, ox, oy, boxW, boxH);
  int y = oy + 2;
  for (int i = 0; i < n; i++) {
    int w = font3x5::textWidth(lines[i], scale);
    RGB c = i == 0 ? RGB(255, 255, 255) : RGB(180, 180, 180);
    font3x5::drawText(ctx.fb, lines[i], ox + (boxW - w) / 2, y, scale, c);
    y += lineH;
  }
}

// draw the configured overlay on top of the current frame
inline void draw(Context& ctx, const Settings& s) {
  switch (s.overlayType) {
    case 1:
      drawDigital(ctx, s.overlaySize == 0 ? 1 : (s.overlaySize == 1 ? 2 : 3),
                  s.overlayPos);
      break;
    case 2:
      drawAnalog(ctx, s.overlaySize == 0 ? 9 : (s.overlaySize == 1 ? 15 : 22),
                 s.overlayPos);
      break;
    case 3:
      drawWord(ctx, s.overlaySize == 2 ? 2 : 1, s.overlayPos);
      break;
    default:
      break;  // off
  }
}

inline const char* typeName(uint8_t t) {
  return t == 0 ? "OFF" : (t == 1 ? "DIGITAL" : (t == 2 ? "ANALOG" : "WORD"));
}

}
}

#endif
