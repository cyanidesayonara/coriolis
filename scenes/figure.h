// The guide-family figure renderer, shared by yoga and exercise. A pose is
// 15 joints in normalized 0..1 space (head, neck, shoulders, elbows,
// wrists, pelvis, knees, ankles, toes) plus a facing direction; the body
// type comes from settings.
#ifndef CORIOLIS_FIGURE_H
#define CORIOLIS_FIGURE_H

#include <math.h>

#include "../core/scene.h"
#include "guide_ui.h"

namespace coriolis {
namespace guide {

// the square region the unit pose space maps into
inline int figureSpan(Context& ctx) {
  int size = ctx.fb.width() < ctx.fb.height() ? ctx.fb.width()
                                              : ctx.fb.height();
  return size - 14;
}

inline void figureOrigin(Context& ctx, int span, int& ox, int& oy) {
  ox = (ctx.fb.width() - span) / 2;
  oy = (ctx.fb.height() - span) / 2 + 3;  // leave room for the top bar
}

inline void tweenPose(const float a[15][2], const float b[15][2], float t,
                      float out[15][2]) {
  for (int i = 0; i < 15; i++) {
    out[i][0] = a[i][0] + (b[i][0] - a[i][0]) * t;
    out[i][1] = a[i][1] + (b[i][1] - a[i][1]) * t;
  }
}

// body: 0 female, 1 male
inline void drawFigure(Context& ctx, const float j[15][2], int face,
                       uint8_t bodyType) {
  int span = figureSpan(ctx);
  int ox, oy;
  figureOrigin(ctx, span, ox, oy);

  int px[15], py[15];
  for (int i = 0; i < 15; i++) {
    px[i] = ox + int(j[i][0] * span);
    py[i] = oy + int(j[i][1] * span);
  }

  RGB body = bodyColor();
  RGB limbs = limbColor();
  RGB hair(115, 68, 38);

  // the mat grounds every pose
  int matY = oy + int(0.93f * span);
  ctx.fb.rect(ox + span / 10, matY, span - span / 5, 2, matColor());

  float smx, smy;  // torso top midpoint, needed by the neck
  {
    // torso: a quad with real width in every view; the width comes from
    // the perpendicular of the shoulders->pelvis axis with enforced
    // minimums so side poses don't collapse to a sliver
    smx = (px[2] + px[3]) * 0.5f;
    smy = (py[2] + py[3]) * 0.5f;
    float axx = px[8] - smx, axy = py[8] - smy;
    float alen = sqrtf(axx * axx + axy * axy);
    if (alen < 1.0f) alen = 1.0f;
    float perpX = -axy / alen, perpY = axx / alen;

    float dxs = float(px[2] - px[3]), dys = float(py[2] - py[3]);
    float shoHalf = 0.5f * sqrtf(dxs * dxs + dys * dys);
    float minSho = span * 0.050f, maxSho = span * 0.095f;
    if (shoHalf < minSho) shoHalf = minSho;
    if (shoHalf > maxSho) shoHalf = maxSho;
    float hipHalf = span * (bodyType == 0 ? 0.048f : 0.038f);

    int sx0 = int(smx + perpX * shoHalf), sy0 = int(smy + perpY * shoHalf);
    int sx1 = int(smx - perpX * shoHalf), sy1 = int(smy - perpY * shoHalf);
    int hx0 = int(px[8] + perpX * hipHalf), hy0 = int(py[8] + perpY * hipHalf);
    int hx1 = int(px[8] - perpX * hipHalf), hy1 = int(py[8] - perpY * hipHalf);

    ctx.fb.fillTriangle(sx0, sy0, sx1, sy1, hx0, hy0, body);
    ctx.fb.fillTriangle(sx1, sy1, hx1, hy1, hx0, hy0, body);

    ctx.fb.fillCircle(int(smx), int(smy), 2, body);
    ctx.fb.fillCircle(px[8], py[8], int(hipHalf), body);

    // neck: snap to the head's column when it's essentially vertical, so
    // integer rounding can't kink it off-center
    int dxn = px[0] - int(smx);
    if (dxn >= -1 && dxn <= 1)
      ctx.fb.boldLine(px[0], int(smy), px[0], py[0], body);
    else
      ctx.fb.boldLine(int(smx), int(smy), px[0], py[0], body);
  }

  // arms: shoulder -> elbow -> wrist, deltoid roots, elbow caps, fists
  ctx.fb.thickLine(px[2], py[2], px[4], py[4], limbs);
  ctx.fb.thickLine(px[4], py[4], px[5], py[5], limbs);
  ctx.fb.thickLine(px[3], py[3], px[6], py[6], limbs);
  ctx.fb.thickLine(px[6], py[6], px[7], py[7], limbs);
  ctx.fb.fillCircle(px[2], py[2], 2, limbs);
  ctx.fb.fillCircle(px[3], py[3], 2, limbs);
  ctx.fb.fillCircle(px[4], py[4], 1, limbs);
  ctx.fb.fillCircle(px[6], py[6], 1, limbs);
  ctx.fb.fillCircle(px[5], py[5], 2, limbs);
  ctx.fb.fillCircle(px[7], py[7], 2, limbs);

  // legs: beefier than arms; feet run ankle -> toe so flexed vs pointed is
  // decided by the pose data
  ctx.fb.boldLine(px[8], py[8], px[9], py[9], limbs);
  ctx.fb.boldLine(px[9], py[9], px[10], py[10], limbs);
  ctx.fb.boldLine(px[8], py[8], px[11], py[11], limbs);
  ctx.fb.boldLine(px[11], py[11], px[12], py[12], limbs);
  ctx.fb.fillCircle(px[9], py[9], 2, limbs);
  ctx.fb.fillCircle(px[11], py[11], 2, limbs);
  ctx.fb.thickLine(px[10], py[10], px[13], py[13], limbs);
  ctx.fb.thickLine(px[12], py[12], px[14], py[14], limbs);

  // head: hair circle behind, face circle in front and slightly lower —
  // the offset leaves a natural hair cap; women get more of it
  int headR = span / 16;
  if (headR < 3) headR = 3;
  int faceOff = bodyType == 0 ? 3 : 2;
  ctx.fb.fillCircle(px[0], py[0], headR, hair);
  int fx = px[0] + face;  // the face sits toward the looking direction
  int fy = py[0] + faceOff;
  ctx.fb.fillCircle(fx, fy, headR - 1, body);

  if (bodyType == 0) {
    if (face == 0) {
      // long hair framing the face
      ctx.fb.rect(px[0] - headR, py[0] - 1, 2, headR + 1, hair);
      ctx.fb.rect(px[0] + headR - 1, py[0] - 1, 2, headR + 1, hair);
    } else {
      // low bun at the back of the head
      ctx.fb.fillCircle(px[0] - face * headR, py[0] + 2, 2, hair);
    }
  }

  // face: eyes and a mouth head-on; eye, nose and mouth in profile
  RGB eyeColor(70, 30, 30);
  if (face == 0) {
    ctx.fb.set(fx - 2, fy - 1, eyeColor);
    ctx.fb.set(fx + 2, fy - 1, eyeColor);
    ctx.fb.hLine(fx - 1, fx + 1, fy + 3, eyeColor);
  } else {
    ctx.fb.set(fx + face * 3, fy - 1, eyeColor);
    ctx.fb.set(fx + face * (headR - 2), fy + 1, eyeColor);
    ctx.fb.set(fx + face * 3, fy + 3, eyeColor);
  }
}

// a kettlebell hanging from (or held at) the midpoint of the two wrists,
// given in normalized pose space
inline void drawKettlebell(Context& ctx, const float j[15][2]) {
  int span = figureSpan(ctx);
  int ox, oy;
  figureOrigin(ctx, span, ox, oy);

  int bx = ox + int((j[5][0] + j[7][0]) * 0.5f * span);
  int by = oy + int((j[5][1] + j[7][1]) * 0.5f * span);

  RGB iron(75, 75, 85);
  RGB ironLight(110, 110, 122);
  // handle arcs over the ball, gripped by the fists
  ctx.fb.rect(bx - 2, by, 5, 2, ironLight);
  ctx.fb.set(bx - 2, by + 2, ironLight);
  ctx.fb.set(bx + 2, by + 2, ironLight);
  ctx.fb.fillCircle(bx, by + 5, 3, iron);
}

}
}

#endif
