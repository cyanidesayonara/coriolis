// Calendar — offline countdowns to yearly events (birthdays, holidays) from
// a plain text file: "DD.MM LABEL" per line. Shows the days remaining to the
// nearest event plus the next couple, and celebrates the day itself with
// fireworks. No network: the events live on the SD card / next to the exe.
#ifndef CORIOLIS_SCENE_CALENDAR_H
#define CORIOLIS_SCENE_CALENDAR_H

#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"
#include "../core/events.h"

namespace coriolis {

class CalendarScene : public Scene {
 public:
  explicit CalendarScene(EventSource& events) : events_(events) {}

  const char* name() const { return "Calendar"; }

  void start(Context&) {
    for (int i = 0; i < MAXP; i++) sparks_[i].life = 0;
    lastBurstMs_ = 0;
  }

  uint32_t draw(Context& ctx) {
    ctx.fb.clear();

    if (events_.count() == 0) {
      const char* msg = "NO EVENTS";
      int w = font3x5::textWidth(msg, 2);
      font3x5::drawText(ctx.fb, msg, (ctx.fb.width() - w) / 2,
                        ctx.fb.height() / 2 - 10, 2, RGB(150, 150, 150));
      const char* hint = "ADD EVENTS.TXT";
      int hw = font3x5::textWidth(hint, 1);
      font3x5::drawText(ctx.fb, hint, (ctx.fb.width() - hw) / 2,
                        ctx.fb.height() / 2 + 6, 1, RGB(90, 90, 90));
      return 250;
    }

    TimeOfDay t = ctx.time.now();
    int today = dayOfYear(t.month, t.day);

    // sort indices by days-remaining (tiny N: selection is fine)
    int order[MAXE];
    int n = events_.count();
    if (n > MAXE) n = MAXE;
    for (int i = 0; i < n; i++) order[i] = i;
    for (int a = 0; a < n - 1; a++)
      for (int b = a + 1; b < n; b++)
        if (daysUntil(order[b], today) < daysUntil(order[a], today)) {
          int tmp = order[a]; order[a] = order[b]; order[b] = tmp;
        }

    const Event& nearest = events_.get(order[0]);
    int days = daysUntil(order[0], today);

    if (days == 0) {
      celebrate(ctx, nearest.label);
      return 33;
    }

    // big countdown: days number, then "DAYS TO <LABEL>"
    char num[8];
    snprintf(num, sizeof(num), "%d", days);
    int scale = days < 100 ? 5 : 4;
    int nw = font3x5::textWidth(num, scale);
    RGB hi = ctx.palette->lookupBright(uint8_t(ctx.nowMs / 900));
    font3x5::drawText(ctx.fb, num, (ctx.fb.width() - nw) / 2, 22, scale, hi);

    char line[28];
    snprintf(line, sizeof(line), "DAYS TO %s", nearest.label);
    int lw = font3x5::textWidth(line, 1);
    font3x5::drawText(ctx.fb, line, (ctx.fb.width() - lw) / 2, 26 + scale * 6,
                      1, RGB(170, 170, 170));

    // the next couple of events, small at the bottom
    int y = ctx.fb.height() - 22;
    for (int i = 1; i < n && i < 3; i++) {
      const Event& e = events_.get(order[i]);
      snprintf(line, sizeof(line), "%d.%d %s %d", e.day, e.month, e.label,
               daysUntil(order[i], today));
      font3x5::drawText(ctx.fb, line, 4, y, 1, RGB(100, 100, 100));
      y += 8;
    }

    // today's date, small in the corner
    snprintf(line, sizeof(line), "%d.%d", t.day, t.month);
    font3x5::drawText(ctx.fb, line, 3, 3, 1, RGB(90, 90, 90));

    return 200;
  }

 private:
  static const int MAXE = 48;
  static const int MAXP = 60;

  struct Spark { float x, y, vx, vy; int life; uint8_t hue; };

  EventSource& events_;
  Spark sparks_[MAXP];
  uint32_t lastBurstMs_;

  static int dayOfYear(int m, int d) {
    static const int cum[12] = {0,   31,  59,  90,  120, 151,
                                181, 212, 243, 273, 304, 334};
    return cum[m - 1] + d;
  }

  int daysUntil(int idx, int today) {
    const Event& e = events_.get(idx);
    int diff = dayOfYear(e.month, e.day) - today;
    if (diff < 0) diff += 365;
    return diff;
  }

  void celebrate(Context& ctx, const char* label) {
    // fireworks: a fresh burst every so often, particles fall with gravity
    if (ctx.nowMs - lastBurstMs_ > 800) {
      lastBurstMs_ = ctx.nowMs;
      float bx = float(20 + randomInt(ctx.fb.width() - 40));
      float by = float(12 + randomInt(ctx.fb.height() / 2));
      uint8_t hue = random8();
      int placed = 0;
      for (int i = 0; i < MAXP && placed < 22; i++) {
        if (sparks_[i].life > 0) continue;
        uint8_t a = random8();
        float sp = 0.5f + random8(0, 100) / 90.0f;
        sparks_[i].x = bx;
        sparks_[i].y = by;
        sparks_[i].vx = (int(sin8(a)) - 128) / 128.0f * sp;
        sparks_[i].vy = (int(cos8(a)) - 128) / 128.0f * sp;
        sparks_[i].life = 26 + random8(20);
        sparks_[i].hue = uint8_t(hue + random8(24));
        placed++;
      }
      ctx.audio.play(Cue::Pop);
    }

    for (int i = 0; i < MAXP; i++) {
      Spark& s = sparks_[i];
      if (s.life <= 0) continue;
      s.life--;
      s.x += s.vx;
      s.y += s.vy;
      s.vy += 0.04f;  // gravity
      RGB c = ctx.palette->lookupBright(s.hue);
      c.dim(uint8_t(60 + s.life * 4));
      ctx.fb.set(int(s.x), int(s.y), c);
    }

    // the celebrated name, blinking
    if ((ctx.nowMs / 450) % 3 != 0) {
      int scale = font3x5::textWidth(label, 2) <= ctx.fb.width() - 8 ? 2 : 1;
      int w = font3x5::textWidth(label, scale);
      font3x5::drawText(ctx.fb, label, (ctx.fb.width() - w) / 2,
                        ctx.fb.height() / 2 - 3, scale, RGB(255, 255, 255));
    }
  }
};

}

#endif
