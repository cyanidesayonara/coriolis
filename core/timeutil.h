// Calendar arithmetic and time zones, with no dependency on any platform
// time library: civil date <-> day count, weekday, and the EU summer-time
// rule. The device RTC keeps UTC and the display shows local time, so the
// clock changes for daylight saving on its own twice a year.
#ifndef CORIOLIS_TIMEUTIL_H
#define CORIOLIS_TIMEUTIL_H

#include <stdint.h>

namespace coriolis {
namespace timeutil {

// days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant)
inline int32_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  int32_t era = (y >= 0 ? y : y - 399) / 400;
  int32_t yoe = y - era * 400;
  int32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

inline void civilFromDays(int32_t z, int& y, int& m, int& d) {
  z += 719468;
  int32_t era = (z >= 0 ? z : z - 146096) / 146097;
  int32_t doe = z - era * 146097;
  int32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  int32_t mp = (5 * doy + 2) / 153;
  d = int(doy - (153 * mp + 2) / 5 + 1);
  m = int(mp < 10 ? mp + 3 : mp - 9);
  y = int(yoe + era * 400 + (m <= 2));
}

// 0 = Sunday .. 6 = Saturday (1970-01-01 was a Thursday)
inline int weekday(int32_t days) {
  int w = int((days + 4) % 7);
  return w < 0 ? w + 7 : w;
}

inline int weekdayOf(int y, int m, int d) {
  return weekday(daysFromCivil(y, m, d));
}

inline int daysInMonth(int y, int m) {
  static const uint8_t n[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
  return (m == 2 && leap) ? 29 : n[(m - 1) % 12];
}

inline const char* weekdayShort(int w) {
  static const char* names[7] = {"Sun", "Mon", "Tue", "Wed",
                                 "Thu", "Fri", "Sat"};
  return names[w % 7];
}

inline const char* monthShort(int m) {
  static const char* names[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return names[(m - 1) % 12];
}

// seconds since the epoch for a UTC civil time
inline int64_t toEpoch(int y, int mo, int d, int h, int mi, int s) {
  return int64_t(daysFromCivil(y, mo, d)) * 86400 + h * 3600 + mi * 60 + s;
}

inline void fromEpoch(int64_t t, int& y, int& mo, int& d, int& h, int& mi,
                      int& s) {
  int32_t days = int32_t(t >= 0 ? t / 86400 : (t - 86399) / 86400);
  int32_t rem = int32_t(t - int64_t(days) * 86400);
  civilFromDays(days, y, mo, d);
  h = rem / 3600;
  mi = (rem / 60) % 60;
  s = rem % 60;
}

inline int32_t lastSundayOf(int y, int m) {
  int32_t last = daysFromCivil(y, m, daysInMonth(y, m));
  return last - weekday(last);
}

// EU rule: summer time from 01:00 UTC on the last Sunday of March until
// 01:00 UTC on the last Sunday of October, everywhere at the same instant
inline bool euSummerTime(int64_t utc) {
  int y, mo, d, h, mi, s;
  fromEpoch(utc, y, mo, d, h, mi, s);
  int64_t start = int64_t(lastSundayOf(y, 3)) * 86400 + 3600;
  int64_t end = int64_t(lastSundayOf(y, 10)) * 86400 + 3600;
  return utc >= start && utc < end;
}

// local = UTC + zone (+1h in summer when the EU rule is on)
inline int32_t localOffset(int64_t utc, int8_t zoneHours, bool euDst) {
  int32_t off = int32_t(zoneHours) * 3600;
  if (euDst && euSummerTime(utc)) off += 3600;
  return off;
}

}
}

#endif
