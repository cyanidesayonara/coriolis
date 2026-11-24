// The guide-family figure renderer, shared by yoga and exercise. A pose is
// 15 joints in normalized 0..1 space (head, neck, shoulders, elbows,
// wrists, pelvis, knees, ankles, toes) plus a facing direction and optional
// spine curvature; the body type comes from settings.
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

namespace detail {

struct Shade {
  RGB base, hi, sh;
};

inline RGB shadeColor(const Shade& s, float light) {
  if (light >= 0.0f) {
    float k = light > 1.0f ? 1.0f : light;
    return lerpColor(s.base, s.hi, uint8_t(k * 255.0f + 0.5f));
  }
  float k = -light;
  if (k > 1.0f) k = 1.0f;
  return lerpColor(s.base, s.sh, uint8_t(k * 255.0f + 0.5f));
}

// smooth tapered capsule with 1px SDF edge anti-aliasing and directional shading
inline void fillCapsule(FrameBuffer& fb, float x0, float y0, float r0, float x1,
                        float y1, float r1, const Shade& s) {
  float pad = (r0 > r1 ? r0 : r1) + 1.5f;
  int minX = int(floorf((x0 < x1 ? x0 : x1) - pad));
  int maxX = int(ceilf((x0 > x1 ? x0 : x1) + pad));
  int minY = int(floorf((y0 < y1 ? y0 : y1) - pad));
  int maxY = int(ceilf((y0 > y1 ? y0 : y1) + pad));
  if (minX < 0) minX = 0;
  if (minY < 0) minY = 0;
  if (maxX >= fb.width()) maxX = fb.width() - 1;
  if (maxY >= fb.height()) maxY = fb.height() - 1;

  float dx = x1 - x0, dy = y1 - y0;
  float len2 = dx * dx + dy * dy;

  for (int y = minY; y <= maxY; y++) {
    float py = y + 0.5f;
    for (int x = minX; x <= maxX; x++) {
      float px = x + 0.5f;
      float t = 0.0f;
      if (len2 > 1e-4f) {
        t = ((px - x0) * dx + (py - y0) * dy) / len2;
        if (t < 0.0f) t = 0.0f;
        else if (t > 1.0f) t = 1.0f;
      }
      float cx = x0 + dx * t, cy = y0 + dy * t;
      float vx = px - cx, vy = py - cy;
      float dist = sqrtf(vx * vx + vy * vy);
      float r = r0 + (r1 - r0) * t;
      float cov = r + 0.5f - dist;
      if (cov <= 0.0f) continue;
      if (cov > 1.0f) cov = 1.0f;
      float nr = r > 0.25f ? r : 0.25f;
      float light = (-0.6f * vx - 0.8f * vy) / nr;
      RGB col = shadeColor(s, light);
      if (cov >= 0.999f) fb.set(x, y, col);
      else fb.blend(x, y, col, uint8_t(cov * 255.0f + 0.5f));
    }
  }
}

// oriented anti-aliased ellipse aligned with unit up-axis (ux, uy)
inline void fillOval(FrameBuffer& fb, float cx, float cy, float rx, float ry,
                     float ux, float uy, const Shade& s) {
  float mr = (rx > ry ? rx : ry) + 1.5f;
  int minX = int(floorf(cx - mr)), maxX = int(ceilf(cx + mr));
  int minY = int(floorf(cy - mr)), maxY = int(ceilf(cy + mr));
  if (minX < 0) minX = 0;
  if (minY < 0) minY = 0;
  if (maxX >= fb.width()) maxX = fb.width() - 1;
  if (maxY >= fb.height()) maxY = fb.height() - 1;

  float axX = -uy, axY = ux;  // local right axis
  float invRx2 = 1.0f / (rx * rx), invRy2 = 1.0f / (ry * ry);

  for (int y = minY; y <= maxY; y++) {
    float dy = (y + 0.5f) - cy;
    for (int x = minX; x <= maxX; x++) {
      float dx = (x + 0.5f) - cx;
      float lx = dx * axX + dy * axY;
      float ly = dx * ux + dy * uy;
      float e2 = lx * lx * invRx2 + ly * ly * invRy2;
      if (e2 > 2.2f) continue;
      float e = sqrtf(e2);
      float gx = lx * invRx2, gy = ly * invRy2;
      float grad = sqrtf(gx * gx + gy * gy);
      float minRad = rx < ry ? rx : ry;
      float dist = (e > 1e-4f && grad > 1e-4f) ? (e - 1.0f) * e / grad : -minRad;
      float cov = 0.5f - dist;
      if (cov <= 0.0f) continue;
      if (cov > 1.0f) cov = 1.0f;
      float maxRad = rx > ry ? rx : ry;
      float light = (-0.6f * dx - 0.8f * dy) / (maxRad > 0.5f ? maxRad : 0.5f);
      RGB col = shadeColor(s, light);
      if (cov >= 0.999f) fb.set(x, y, col);
      else fb.blend(x, y, col, uint8_t(cov * 255.0f + 0.5f));
    }
  }
}

inline void interpBone(const float pA[2], const float cA[2], const float pB[2],
                       const float cB[2], const float pOut[2], float t,
                       float cOut[2]) {
  float dxA = cA[0] - pA[0], dyA = cA[1] - pA[1];
  float dxB = cB[0] - pB[0], dyB = cB[1] - pB[1];
  float lenA = sqrtf(dxA * dxA + dyA * dyA);
  float lenB = sqrtf(dxB * dxB + dyB * dyB);
  if (lenA < 1e-4f || lenB < 1e-4f) {
    float rx = dxA + (dxB - dxA) * t;
    float ry = dyA + (dyB - dyA) * t;
    cOut[0] = pOut[0] + rx;
    cOut[1] = pOut[1] + ry;
    return;
  }
  float angA = atan2f(dyA, dxA);
  float angB = atan2f(dyB, dxB);
  float dAng = angB - angA;
  while (dAng > 3.14159265f) dAng -= 6.28318531f;
  while (dAng < -3.14159265f) dAng += 6.28318531f;
  float ang = angA + dAng * t;
  float len = lenA + (lenB - lenA) * t;
  cOut[0] = pOut[0] + cosf(ang) * len;
  cOut[1] = pOut[1] + sinf(ang) * len;
}

inline void solvePlantedLeg(const float a[15][2], const float b[15][2], float t,
                            int hip, int knee, int ank, int toe,
                            float out[15][2]) {
  out[ank][0] = a[ank][0] + (b[ank][0] - a[ank][0]) * t;
  out[ank][1] = a[ank][1] + (b[ank][1] - a[ank][1]) * t;
  out[toe][0] = a[toe][0] + (b[toe][0] - a[toe][0]) * t;
  out[toe][1] = a[toe][1] + (b[toe][1] - a[toe][1]) * t;

  float l1A = hypotf(a[knee][0] - a[hip][0], a[knee][1] - a[hip][1]);
  float l1B = hypotf(b[knee][0] - b[hip][0], b[knee][1] - b[hip][1]);
  float l2A = hypotf(a[ank][0] - a[knee][0], a[ank][1] - a[knee][1]);
  float l2B = hypotf(b[ank][0] - b[knee][0], b[ank][1] - b[knee][1]);
  float l1 = l1A + (l1B - l1A) * t;
  float l2 = l2A + (l2B - l2A) * t;

  float vx = out[ank][0] - out[hip][0], vy = out[ank][1] - out[hip][1];
  float origD = sqrtf(vx * vx + vy * vy);
  if (origD < 1e-4f || l1 < 1e-4f || l2 < 1e-4f) {
    out[knee][0] = a[knee][0] + (b[knee][0] - a[knee][0]) * t;
    out[knee][1] = a[knee][1] + (b[knee][1] - a[knee][1]) * t;
    return;
  }

  float crossA = (a[knee][0] - a[hip][0]) * (a[ank][1] - a[hip][1]) -
                 (a[knee][1] - a[hip][1]) * (a[ank][0] - a[hip][0]);
  float crossB = (b[knee][0] - b[hip][0]) * (b[ank][1] - b[hip][1]) -
                 (b[knee][1] - b[hip][1]) * (b[ank][0] - b[hip][0]);
  float pick = fabsf(crossB) > fabsf(crossA) ? crossB : crossA;
  if (fabsf(pick) < 1e-5f) {
    out[knee][0] = a[knee][0] + (b[knee][0] - a[knee][0]) * t;
    out[knee][1] = a[knee][1] + (b[knee][1] - a[knee][1]) * t;
    return;
  }

  float ux = vx / origD, uy = vy / origD;
  if (origD >= l1 + l2) {
    float frac = l1 / (l1 + l2);
    out[knee][0] = out[hip][0] + ux * (origD * frac);
    out[knee][1] = out[hip][1] + uy * (origD * frac);
    return;
  }

  float minReach = fabsf(l1 - l2) + 1e-4f;
  float d = origD < minReach ? minReach : origD;
  float cosA = (l1 * l1 + d * d - l2 * l2) / (2.0f * l1 * d);
  if (cosA > 1.0f) cosA = 1.0f;
  if (cosA < -1.0f) cosA = -1.0f;
  float sinA = sqrtf(1.0f - cosA * cosA);
  float bendSign = pick >= 0.0f ? -1.0f : 1.0f;
  out[knee][0] = out[hip][0] + ux * (cosA * l1) + (-uy) * (bendSign * sinA * l1);
  out[knee][1] = out[hip][1] + uy * (cosA * l1) + ux * (bendSign * sinA * l1);
}

inline void drawArm(FrameBuffer& fb, float sx, float sy, float ex, float ey,
                    float wx, float wy, float span, uint8_t bodyType,
                    const Shade& skin, const Shade& top) {
  float rSho = span * (bodyType == 0 ? 0.025f : 0.028f);
  float rElb = span * 0.021f;
  float rWri = span * 0.015f;
  fillCapsule(fb, sx, sy, rSho, ex, ey, rElb, skin);
  if (bodyType == 1) {
    // short t-shirt sleeve on upper 38% of upper arm
    float slX = sx + (ex - sx) * 0.38f;
    float slY = sy + (ey - sy) * 0.38f;
    float slR = rSho + (rElb - rSho) * 0.38f + span * 0.004f;
    fillCapsule(fb, sx, sy, rSho + span * 0.004f, slX, slY, slR, top);
  }
  fillCapsule(fb, ex, ey, rElb, wx, wy, rWri, skin);
  fb.fillCircleAA(wx, wy, span * 0.018f, skin.base);
}

inline void drawLeg(FrameBuffer& fb, float hx, float hy, float kx, float ky,
                    float ax, float ay, float tx, float ty, float span,
                    uint8_t bodyType, const Shade& skin, const Shade& bottom,
                    const Shade& shoe) {
  float rHip = span * (bodyType == 0 ? 0.042f : 0.040f);
  float rKnee = span * 0.029f;
  float rAnk = span * 0.019f;
  float rToe = span * 0.015f;

  if (bodyType == 0) {
    // full-length leggings + shoes
    fillCapsule(fb, hx, hy, rHip, kx, ky, rKnee, bottom);
    fillCapsule(fb, kx, ky, rKnee, ax, ay, rAnk, bottom);
  } else {
    // athletic shorts down to 62% of thigh, bare lower thigh/calf
    fillCapsule(fb, hx, hy, rHip, kx, ky, rKnee, skin);
    float shX = hx + (kx - hx) * 0.62f;
    float shY = hy + (ky - hy) * 0.62f;
    float shR = rHip + (rKnee - rHip) * 0.62f + span * 0.004f;
    fillCapsule(fb, hx, hy, rHip + span * 0.003f, shX, shY, shR, bottom);
    fillCapsule(fb, kx, ky, rKnee, ax, ay, rAnk, skin);
  }
  fillCapsule(fb, ax, ay, rAnk, tx, ty, rToe, shoe);
}

inline void drawTorso(FrameBuffer& fb, float smx, float smy, float px8,
                      float py8, float shoHalf, float span, int face,
                      uint8_t bodyType, float spineCurve, const Shade& top,
                      const Shade& bottom, const Shade& skin) {
  float dx = px8 - smx, dy = py8 - smy;
  float L = sqrtf(dx * dx + dy * dy);
  if (L < span * 0.08f) L = span * 0.08f;
  float ux = dx / L, uy = dy / L;
  float rx = -uy, ry = ux;

  float wChest = face == 0
                     ? (shoHalf * (bodyType == 0 ? 0.82f : 0.90f))
                     : span * (bodyType == 0 ? 0.050f : 0.055f);
  float minChest = span * (bodyType == 0 ? 0.052f : 0.058f);
  if (wChest < minChest) wChest = minChest;
  float wWaist = span * (bodyType == 0 ? 0.040f : 0.047f);
  float wHip = span * (bodyType == 0 ? 0.056f : 0.047f);
  float split = bodyType == 0 ? 0.56f : 0.62f;

  float capR = span * 0.022f;
  float botR = wHip * 0.65f;
  float curvePx = spineCurve * span * (face < 0 ? -1.0f : (face > 0 ? 1.0f : 0.0f));

  float maxW = (wChest > wHip ? wChest : wHip) + fabsf(curvePx) + 3.0f;
  float minX = (smx < px8 ? smx : px8) - maxW;
  float maxX = (smx > px8 ? smx : px8) + maxW;
  float minY = (smy < py8 ? smy : py8) - maxW;
  float maxY = (smy > py8 ? smy : py8) + maxW;

  int x0 = int(floorf(minX)), x1 = int(ceilf(maxX));
  int y0 = int(floorf(minY)), y1 = int(ceilf(maxY));
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 >= fb.width()) x1 = fb.width() - 1;
  if (y1 >= fb.height()) y1 = fb.height() - 1;

  for (int y = y0; y <= y1; y++) {
    float py = y + 0.5f;
    for (int x = x0; x <= x1; x++) {
      float px = x + 0.5f;
      float vx = px - smx, vy = py - smy;
      float alongPx = vx * ux + vy * uy;
      float along = alongPx / L;
      if (alongPx < -capR - 1.2f || alongPx > L + botR + 1.2f) continue;

      float ac = along < 0.0f ? 0.0f : (along > 1.0f ? 1.0f : along);
      float hw;
      if (ac <= 0.52f) {
        float k = ac / 0.52f;
        k = k * k * (3.0f - 2.0f * k);
        hw = wChest + (wWaist - wChest) * k;
      } else {
        float k = (ac - 0.52f) / 0.48f;
        k = k * k * (3.0f - 2.0f * k);
        hw = wWaist + (wHip - wWaist) * k;
      }
      float curveOff = sinf(ac * 3.14159265f) * curvePx;
      float side = (vx * rx + vy * ry) - curveOff;

      float sd;
      if (alongPx < 0.0f) {
        float ex = fabsf(side) - (hw - capR);
        if (ex < 0.0f) ex = 0.0f;
        float ey = -alongPx;
        sd = sqrtf(ex * ex + ey * ey) - capR;
      } else if (alongPx > L) {
        float ex = fabsf(side) - (hw - botR > 0.0f ? hw - botR : 0.0f);
        if (ex < 0.0f) ex = 0.0f;
        float ey = alongPx - L;
        sd = sqrtf(ex * ex + ey * ey) - botR;
      } else {
        sd = fabsf(side) - hw;
      }

      float cov = 0.5f - sd;
      if (cov <= 0.0f) continue;
      if (cov > 1.0f) cov = 1.0f;

      // 3D cylinder light across the torso width + soft cap shading
      float lat = hw > 0.25f ? side / hw : 0.0f;
      if (lat > 1.0f) lat = 1.0f;
      if (lat < -1.0f) lat = -1.0f;
      float nx = lat * rx, ny = lat * ry;
      if (alongPx < 0.0f) { nx -= ux * 0.4f; ny -= uy * 0.4f; }
      float light = -0.6f * nx - 0.8f * ny;

      // clothing color: coral top, periwinkle bottoms, neckline on female front
      RGB col;
      if (bodyType == 0 && face == 0 && along < 0.14f &&
          fabsf(side) < span * 0.026f * (1.0f - along / 0.14f)) {
        col = shadeColor(skin, light);
      } else {
        float blendPx = (along - split) * L;
        if (blendPx <= -0.6f) {
          col = shadeColor(top, light);
        } else if (blendPx >= 0.6f) {
          col = shadeColor(bottom, light);
        } else {
          float f = (blendPx + 0.6f) / 1.2f;
          col = lerpColor(shadeColor(top, light), shadeColor(bottom, light),
                          uint8_t(f * 255.0f + 0.5f));
        }
      }

      if (cov >= 0.999f) fb.set(x, y, col);
      else fb.blend(x, y, col, uint8_t(cov * 255.0f + 0.5f));
    }
  }
}

}  // namespace detail

// Arc-preserving kinematic pose interpolation: interpolates pelvis linearly,
// swings torso and arm bones by shortest angle while keeping segment lengths
// true, and uses analytical 2-bone IK when an ankle stays planted on the mat.
inline void tweenPoseKinematic(const float a[15][2], const float b[15][2],
                               float t, float out[15][2]) {
  out[8][0] = a[8][0] + (b[8][0] - a[8][0]) * t;
  out[8][1] = a[8][1] + (b[8][1] - a[8][1]) * t;

  detail::interpBone(a[8], a[1], b[8], b[1], out[8], t, out[1]);
  detail::interpBone(a[1], a[0], b[1], b[0], out[1], t, out[0]);

  detail::interpBone(a[1], a[2], b[1], b[2], out[1], t, out[2]);
  detail::interpBone(a[2], a[4], b[2], b[4], out[2], t, out[4]);
  detail::interpBone(a[4], a[5], b[4], b[5], out[4], t, out[5]);

  detail::interpBone(a[1], a[3], b[1], b[3], out[1], t, out[3]);
  detail::interpBone(a[3], a[6], b[3], b[6], out[3], t, out[6]);
  detail::interpBone(a[6], a[7], b[6], b[7], out[6], t, out[7]);

  // left leg (9, 10, 13)
  if (hypotf(b[10][0] - a[10][0], b[10][1] - a[10][1]) < 0.035f) {
    detail::solvePlantedLeg(a, b, t, 8, 9, 10, 13, out);
  } else {
    detail::interpBone(a[8], a[9], b[8], b[9], out[8], t, out[9]);
    detail::interpBone(a[9], a[10], b[9], b[10], out[9], t, out[10]);
    detail::interpBone(a[10], a[13], b[10], b[13], out[10], t, out[13]);
  }

  // right leg (11, 12, 14)
  if (hypotf(b[12][0] - a[12][0], b[12][1] - a[12][1]) < 0.035f) {
    detail::solvePlantedLeg(a, b, t, 8, 11, 12, 14, out);
  } else {
    detail::interpBone(a[8], a[11], b[8], b[11], out[8], t, out[11]);
    detail::interpBone(a[11], a[12], b[11], b[12], out[11], t, out[12]);
    detail::interpBone(a[12], a[14], b[12], b[14], out[12], t, out[14]);
  }
}

// the other side of a one-sided pose: flip horizontally and swap every
// left/right joint pair so the anatomy stays correct (the left knee is still
// the left knee, it's just forward now)
inline void mirrorPose(const float in[15][2], float out[15][2]) {
  static const uint8_t partner[15] = {0, 1, 3, 2, 6, 7, 4, 5, 8,
                                      11, 12, 9, 10, 14, 13};
  for (int i = 0; i < 15; i++) {
    out[i][0] = 1.0f - in[partner[i]][0];
    out[i][1] = in[partner[i]][1];
  }
}

// body: 0 female, 1 male. Draws into the square (ox, oy, span); the mat
// is optional so small previews can float. spineCurve > 0 arches outward.
inline void drawFigureAt(Context& ctx, const float j[15][2], int face,
                         uint8_t bodyType, int ox, int oy, int span,
                         bool mat = true, float spineCurve = 0.0f) {
  using namespace detail;

  float px[15], py[15];
  for (int i = 0; i < 15; i++) {
    px[i] = ox + j[i][0] * span;
    py[i] = oy + j[i][1] * span;
  }

  const Shade skinNear = {RGB(245, 186, 148), RGB(255, 212, 178),
                          RGB(196, 138, 104)};
  const Shade skinFar = {RGB(185, 134, 104), RGB(210, 158, 125),
                         RGB(138, 94, 70)};
  const Shade topNear = {RGB(242, 108, 92), RGB(255, 152, 130),
                         RGB(178, 64, 58)};
  const Shade topFar = {RGB(182, 74, 66), RGB(208, 106, 92),
                        RGB(130, 44, 40)};
  const Shade botNear = {RGB(112, 134, 214), RGB(150, 172, 242),
                         RGB(72, 88, 154)};
  const Shade botFar = {RGB(74, 90, 152), RGB(98, 116, 182),
                        RGB(46, 58, 106)};
  const Shade shoeNear = {RGB(214, 232, 226), RGB(245, 252, 250),
                          RGB(145, 165, 160)};
  const Shade shoeFar = {RGB(142, 160, 156), RGB(172, 190, 184),
                         RGB(96, 110, 108)};
  const Shade hairShade = {RGB(102, 58, 34), RGB(144, 88, 52),
                           RGB(62, 34, 18)};

  // the mat grounds every pose, with subtle contact shadows underneath
  if (mat) {
    int matX = ox + span / 10;
    int matW = span - span / 5;
    int matY = oy + int(0.93f * span);
    ctx.fb.hLine(matX, matX + matW - 1, matY, RGB(38, 218, 196));
    ctx.fb.hLine(matX, matX + matW - 1, matY + 1, matColor());
    ctx.fb.hLine(matX, matX + matW - 1, matY + 2, RGB(0, 95, 86));

    static const int contactJoints[] = {0, 5, 7, 8, 9, 10, 11, 12, 13, 14};
    for (int k = 0; k < int(sizeof(contactJoints) / sizeof(contactJoints[0])); k++) {
      int idx = contactJoints[k];
      if (j[idx][1] >= 0.85f) {
        int sx = int(px[idx]);
        for (int dx = -3; dx <= 3; dx++) {
          int mx = sx + dx;
          if (mx >= matX && mx < matX + matW) {
            uint8_t keep = uint8_t(125 + (dx < 0 ? -dx : dx) * 35);
            ctx.fb.at(mx, matY).dim(keep);
            ctx.fb.at(mx, matY + 1).dim(keep);
          }
        }
      }
    }
  }

  float smx = (px[2] + px[3]) * 0.5f;
  float smy = (py[2] + py[3]) * 0.5f;
  float shoHalf = 0.5f * hypotf(px[2] - px[3], py[2] - py[3]);

  // hip attachment offsets slightly outward in front view so legs sit under hip sockets
  float hipOff = face == 0 ? span * (bodyType == 0 ? 0.032f : 0.028f) : 0.0f;
  float lhx = px[8] - hipOff, rhx = px[8] + hipOff;
  float hy = py[8];

  // 1. Far limbs (in profile, back side sits behind torso and gets ~72% shade)
  if (face < 0) {
    drawArm(ctx.fb, px[3], py[3], px[6], py[6], px[7], py[7], float(span),
            bodyType, skinFar, topFar);
    drawLeg(ctx.fb, rhx, hy, px[11], py[11], px[12], py[12], px[14], py[14],
            float(span), bodyType, skinFar, botFar, shoeFar);
  } else if (face > 0) {
    drawArm(ctx.fb, px[2], py[2], px[4], py[4], px[5], py[5], float(span),
            bodyType, skinFar, topFar);
    drawLeg(ctx.fb, lhx, hy, px[9], py[9], px[10], py[10], px[13], py[13],
            float(span), bodyType, skinFar, botFar, shoeFar);
  } else {
    drawLeg(ctx.fb, lhx, hy, px[9], py[9], px[10], py[10], px[13], py[13],
            float(span), bodyType, skinNear, botNear, shoeNear);
    drawLeg(ctx.fb, rhx, hy, px[11], py[11], px[12], py[12], px[14], py[14],
            float(span), bodyType, skinNear, botNear, shoeNear);
  }

  // 2. Torso, neck, and head + hair
  drawTorso(ctx.fb, smx, smy, px[8], py[8], shoHalf, float(span), face,
            bodyType, spineCurve, topNear, botNear, skinNear);

  // neck connects smoothly from shoulder midpoint toward head center
  float neckX = fabsf(px[0] - smx) <= 1.5f ? px[0] : smx;
  fillCapsule(ctx.fb, neckX, smy, span * 0.019f, px[0], py[0], span * 0.016f,
              skinNear);

  // oriented head basis: u points top-of-head, fwd points facing direction
  float hx = px[0], hyPos = py[0];
  float ux = px[0] - px[1], uy = py[0] - py[1];
  float uLen = sqrtf(ux * ux + uy * uy);
  if (uLen > 1e-3f) { ux /= uLen; uy /= uLen; }
  else { ux = 0.0f; uy = -1.0f; }
  float fx = face < 0 ? uy : -uy;
  float fy = face < 0 ? -ux : ux;

  float headRy = span * 0.066f;
  if (headRy < 3.0f) headRy = 3.0f;
  float headRx = headRy * 0.86f;

  // hair frame + skull cap behind the face oval, so the face stays clean and open
  if (bodyType == 0) {
    if (face == 0) {
      // soft shoulder-framing hair behind the head
      fillOval(ctx.fb, hx - ux * headRy * 0.14f, hyPos - uy * headRy * 0.14f,
               headRx * 1.18f, headRy * 1.08f, ux, uy, hairShade);
    } else {
      // ponytail / low bun tucked behind the crown
      float bunX = hx - fx * headRx * 1.05f + ux * headRy * 0.15f;
      float bunY = hyPos - fy * headRx * 1.05f + uy * headRy * 0.15f;
      fillOval(ctx.fb, bunX, bunY, headRx * 0.56f, headRy * 0.45f, ux, uy,
               hairShade);
    }
  }

  // skull hair cap sits slightly up and back behind the face oval
  float capX = hx + ux * headRy * 0.18f - (face != 0 ? fx * headRx * 0.22f : 0.0f);
  float capY = hyPos + uy * headRy * 0.18f - (face != 0 ? fy * headRx * 0.22f : 0.0f);
  fillOval(ctx.fb, capX, capY, headRx * 1.02f, headRy * 1.02f, ux, uy,
           hairShade);

  // clean warm skin face oval shifted slightly forward and down
  float shiftF = span >= 80 ? headRx * 0.22f : headRx * 0.18f;
  float shiftD = span >= 80 ? headRy * 0.15f : headRy * 0.12f;
  float hcx = hx - ux * shiftD + (face != 0 ? fx * shiftF : 0.0f);
  float hcy = hyPos - uy * shiftD + (face != 0 ? fy * shiftF : 0.0f);
  float faceRx = headRx * 0.85f;
  float faceRy = headRy * 0.84f;
  fillOval(ctx.fb, hcx, hcy, faceRx, faceRy, ux, uy, skinNear);

  // subtle bangs accent along the very top hairline for female front view
  if (bodyType == 0 && face == 0 && span >= 80) {
    fillOval(ctx.fb, hx + ux * headRy * 0.58f, hyPos + uy * headRy * 0.58f,
             headRx * 0.72f, headRy * 0.30f, ux, uy, hairShade);
  }

  // minimalist calm facial cues on full-size figures only
  if (span >= 80) {
    RGB eyeCol(58, 34, 24);
    if (face == 0) {
      float ex = faceRx * 0.42f;
      float ey = -faceRy * 0.04f;
      float rightX = -uy, rightY = ux;
      ctx.fb.set(int(hcx - rightX * ex + ux * ey + 0.5f),
                 int(hcy - rightY * ex + uy * ey + 0.5f), eyeCol);
      ctx.fb.set(int(hcx + rightX * ex + ux * ey + 0.5f),
                 int(hcy + rightY * ex + uy * ey + 0.5f), eyeCol);
    } else {
      float ex = hcx + fx * faceRx * 0.38f + ux * faceRy * 0.06f;
      float ey = hcy + fy * faceRx * 0.38f + uy * faceRy * 0.06f;
      ctx.fb.set(int(ex + 0.5f), int(ey + 0.5f), eyeCol);
    }
  }

  // 3. Near limbs (frontmost layer)
  if (face < 0) {
    drawLeg(ctx.fb, lhx, hy, px[9], py[9], px[10], py[10], px[13], py[13],
            float(span), bodyType, skinNear, botNear, shoeNear);
    drawArm(ctx.fb, px[2], py[2], px[4], py[4], px[5], py[5], float(span),
            bodyType, skinNear, topNear);
  } else if (face > 0) {
    drawLeg(ctx.fb, rhx, hy, px[11], py[11], px[12], py[12], px[14], py[14],
            float(span), bodyType, skinNear, botNear, shoeNear);
    drawArm(ctx.fb, px[3], py[3], px[6], py[6], px[7], py[7], float(span),
            bodyType, skinNear, topNear);
  } else {
    drawArm(ctx.fb, px[2], py[2], px[4], py[4], px[5], py[5], float(span),
            bodyType, skinNear, topNear);
    drawArm(ctx.fb, px[3], py[3], px[6], py[6], px[7], py[7], float(span),
            bodyType, skinNear, topNear);
  }
}

inline void drawFigure(Context& ctx, const float j[15][2], int face,
                       uint8_t bodyType, float spineCurve = 0.0f) {
  int span = figureSpan(ctx);
  int ox, oy;
  figureOrigin(ctx, span, ox, oy);
  drawFigureAt(ctx, j, face, bodyType, ox, oy, span, true, spineCurve);
}

// a kettlebell hanging from (or held at) the midpoint of the two wrists,
// given in normalized pose space
inline void drawKettlebellAt(Context& ctx, const float j[15][2], int ox,
                             int oy, int span) {
  using namespace detail;
  float bx = ox + (j[5][0] + j[7][0]) * 0.5f * span;
  float by = oy + (j[5][1] + j[7][1]) * 0.5f * span;

  float ballR = span * 0.045f;
  if (ballR < 2.6f) ballR = 2.6f;
  float handleR = span * 0.011f;
  if (handleR < 0.8f) handleR = 0.8f;

  const Shade handleShade = {RGB(124, 136, 152), RGB(170, 184, 200),
                             RGB(70, 78, 92)};
  const Shade ironShade = {RGB(74, 82, 98), RGB(150, 165, 185),
                           RGB(38, 42, 54)};
  const Shade gripSkin = {RGB(245, 186, 148), RGB(255, 212, 178),
                          RGB(196, 138, 104)};

  float hw = ballR * 0.68f;
  float drop = ballR * 0.85f;
  // arched metallic handle
  fillCapsule(ctx.fb, bx - hw, by, handleR, bx + hw, by, handleR, handleShade);
  fillCapsule(ctx.fb, bx - hw, by, handleR, bx - hw * 0.8f, by + drop, handleR,
              handleShade);
  fillCapsule(ctx.fb, bx + hw, by, handleR, bx + hw * 0.8f, by + drop, handleR,
              handleShade);

  // anti-aliased shaded iron bell body
  float cy = by + drop + ballR * 0.7f;
  fillOval(ctx.fb, bx, cy, ballR, ballR * 0.96f, 0.0f, -1.0f, ironShade);

  // hands gripping the top bar cleanly
  ctx.fb.fillCircleAA(bx - hw * 0.45f, by, span * 0.017f, gripSkin.base);
  ctx.fb.fillCircleAA(bx + hw * 0.45f, by, span * 0.017f, gripSkin.base);
}

inline void drawKettlebell(Context& ctx, const float j[15][2]) {
  int span = figureSpan(ctx);
  int ox, oy;
  figureOrigin(ctx, span, ox, oy);
  drawKettlebellAt(ctx, j, ox, oy, span);
}

}  // namespace guide
}  // namespace coriolis

#endif
