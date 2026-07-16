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

}

#endif
