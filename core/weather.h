// Weather comes through a provider interface like time and events. The
// display stays offline: the sim fakes it (or reads weather.txt), and the
// device will get real data pushed over UART by a tiny feeder module —
// or simply shows "waiting" forever, which is also fine.
#ifndef CORIOLIS_WEATHER_H
#define CORIOLIS_WEATHER_H

namespace coriolis {

enum class Wx { Unknown, Clear, Clouds, Rain, Snow, Storm };

struct WeatherInfo {
  Wx condition;
  int tempC;
  bool valid;
};

class WeatherProvider {
 public:
  virtual ~WeatherProvider() {}
  virtual WeatherInfo now() = 0;
};

class NoWeather : public WeatherProvider {
 public:
  WeatherInfo now() {
    WeatherInfo w;
    w.condition = Wx::Unknown;
    w.tempC = 0;
    w.valid = false;
    return w;
  }
};

}

#endif
