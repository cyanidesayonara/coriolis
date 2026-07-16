// Calendar events — the offline tier from the plan. Recurring yearly dates
// (birthdays, holidays, countdowns) come from a source the backend provides:
// a plain events.txt next to the exe in the sim, a file on SD on the device.
// No network anywhere.
#ifndef CORIOLIS_EVENTS_H
#define CORIOLIS_EVENTS_H

namespace coriolis {

struct Event {
  int day;    // 1-31
  int month;  // 1-12
  char label[20];
};

class EventSource {
 public:
  virtual ~EventSource() {}
  virtual int count() = 0;
  virtual const Event& get(int i) = 0;
};

class NoEvents : public EventSource {
 public:
  int count() { return 0; }
  const Event& get(int) {
    static const Event none = {1, 1, ""};
    return none;
  }
};

// Built-in holidays for Barcelona / Badalona (fixed-date ones; the movable
// feasts — Setmana Santa, Pasqua Granada — shift yearly and are left to
// events.txt). Backends append these after the user's own events.
static const Event BCN_HOLIDAYS[] = {
    {1, 1, "CAP D'ANY"},      {6, 1, "REIS"},
    {23, 4, "SANT JORDI"},    {1, 5, "1 DE MAIG"},
    {11, 5, "SANT ANASTASI"},  // Badalona: Festes de Maig
    {24, 6, "SANT JOAN"},     {15, 8, "L'ASSUMPCIO"},
    {11, 9, "LA DIADA"},      {24, 9, "LA MERCE"},
    {12, 10, "HISPANITAT"},   {1, 11, "TOTS SANTS"},
    {6, 12, "CONSTITUCIO"},   {8, 12, "IMMACULADA"},
    {25, 12, "NADAL"},        {26, 12, "SANT ESTEVE"},
};
static const int BCN_HOLIDAY_COUNT =
    int(sizeof(BCN_HOLIDAYS) / sizeof(BCN_HOLIDAYS[0]));

}

#endif
