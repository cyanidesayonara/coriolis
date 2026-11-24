// Coriolis WebAssembly backend — compiles the exact shared core/ and scenes/
// engine (Shell, Launcher, Settings, Transitions, Overlays, and all 25 scenes)
// to WebAssembly via Emscripten for the interactive browser emulator.
//
// Zero raylib or hardware dependencies; renders RGBA pixels into a static
// buffer read directly by site/emu.js.
#include "../core/config.h"
#include "../core/display.h"
#include "../core/transition.h"
#include "../core/scene.h"
#include "../core/settings.h"
#include "../scenes/scene_clock.h"
#include "../scenes/scene_analogclock.h"
#include "../scenes/scene_wordclock.h"
#include "../scenes/scene_calendar.h"
#include "../scenes/scene_pong.h"
#include "../scenes/scene_snake.h"
#include "../scenes/scene_tetris.h"
#include "../scenes/scene_yoga.h"
#include "../scenes/scene_exercise.h"
#include "../scenes/scene_breathe.h"
#include "../scenes/scene_focus.h"
#include "../scenes/scene_weather.h"
#include "../scenes/scene_gifs.h"
#include "../scenes/scene_aurora.h"
#include "../scenes/scene_fire.h"
#include "../scenes/scene_spiro.h"
#include "../scenes/scene_mandala.h"
#include "../scenes/scene_coriolis.h"
#include "../scenes/scene_labyrinth.h"
#include "../scenes/scene_lava.h"
#include "../scenes/scene_rain.h"
#include "../scenes/scene_bounce.h"
#include "../scenes/scene_starfield.h"
#include "../scenes/scene_life.h"
#include "../scenes/scene_aquarium.h"
#include "../scenes/scene_settings.h"
#include "../scenes/clock_overlay.h"
#include "../scenes/toast.h"
#include "../scenes/shell.h"

#include <stdint.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define WEB_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define WEB_EXPORT
#endif

namespace {

using namespace coriolis;

class WebTime : public TimeSource {
 public:
  WebTime() {
    now_.year = 2026;
    now_.month = 11;
    now_.day = 24;
    now_.hour = 12;
    now_.minute = 0;
    now_.second = 0;
  }
  void set(int year, int month, int day, int hour, int minute, int second) {
    now_.year = year;
    now_.month = month;
    now_.day = day;
    now_.hour = hour;
    now_.minute = minute;
    now_.second = second;
  }
  TimeOfDay now() { return now_; }

 private:
  TimeOfDay now_;
};

class WebInput : public InputState {
 public:
  void setHeld(int keyId, bool down) {
    if (keyId < 0 || keyId >= 8) return;
    if (down)
      mask_ |= uint8_t(1u << keyId);
    else
      mask_ &= uint8_t(~(1u << keyId));
  }
  bool isDown(Key k) const {
    int id = int(k);
    if (id <= 0 || id >= 8) return false;
    return (mask_ & uint8_t(1u << id)) != 0;
  }

 private:
  uint8_t mask_ = 0;
};

class WebAudio : public AudioSink {
 public:
  void play(Cue c) {
    if (c == Cue::None) return;
#ifdef __EMSCRIPTEN__
    EM_ASM(
        {
          if (window._coriolisAudio && window._coriolisAudio.play) {
            window._coriolisAudio.play($0);
          }
        },
        int(c));
#else
    (void)c;
#endif
  }

  void voice(int clip) {
#ifdef __EMSCRIPTEN__
    EM_ASM(
        {
          if (window._coriolisAudio && window._coriolisAudio.voice) {
            window._coriolisAudio.voice($0);
          }
        },
        clip);
#else
    (void)clip;
#endif
  }

  void loop(Cue c) {
#ifdef __EMSCRIPTEN__
    EM_ASM(
        {
          if (window._coriolisAudio && window._coriolisAudio.loop) {
            window._coriolisAudio.loop($0);
          }
        },
        int(c));
#else
    (void)c;
#endif
  }

  void setVolume(uint8_t v) {
#ifdef __EMSCRIPTEN__
    EM_ASM(
        {
          if (window._coriolisAudio && window._coriolisAudio.setVolume) {
            window._coriolisAudio.setVolume($0);
          }
        },
        int(v));
#else
    (void)v;
#endif
  }
};

class WebSettingsStore : public SettingsStore {
 public:
  bool load(Settings& out) {
    // Visitors browsing the website late at night should see the display at
    // full daytime vibrancy by default; Night Mode remains a single toggle away.
    out.nightMode = false;
    return true;
  }
  void save(const Settings&) {}
};

class WebEventSource : public EventSource {
 public:
  int count() { return BCN_HOLIDAY_COUNT; }
  const Event& get(int i) {
    if (i < 0) i = 0;
    if (i >= BCN_HOLIDAY_COUNT) i = BCN_HOLIDAY_COUNT - 1;
    return BCN_HOLIDAYS[i];
  }
};

class WebWeatherProvider : public WeatherProvider {
 public:
  WeatherInfo now() {
    static const WeatherInfo PRESETS[] = {
        {Wx::Clear, 22, true},
        {Wx::Clouds, 17, true},
        {Wx::Rain, 13, true},
        {Wx::Snow, -1, true},
        {Wx::Storm, 16, true},
    };
    return PRESETS[index_];
  }

  const char* cycle() {
    static const char* NAMES[] = {"Weather: Clear 22C", "Weather: Clouds 17C",
                                  "Weather: Rain 13C", "Weather: Snow -1C",
                                  "Weather: Storm 16C"};
    index_ = (index_ + 1) % 5;
    return NAMES[index_];
  }

 private:
  int index_ = 0;
};

// Lightweight zero-heap procedural pixel-art loops so the Gifs scene works out
// of the box in the browser without shipping binary image assets.
class DemoGifSource : public GifSource {
 public:
  int count() { return 2; }
  bool open(int index) {
    clip_ = (index & 1);
    frame_ = 0;
    return true;
  }

  uint32_t drawNextFrame(FrameBuffer& fb) {
    const int W = fb.width();
    const int H = fb.height();
    fb.clear();
    uint32_t f = frame_++;

    if (clip_ == 0) {
      // Twinkling night sky over pixel hills + glowing moon
      for (int y = 0; y < H; y++) {
        uint8_t sky = uint8_t(6 + (y * 16) / H);
        for (int x = 0; x < W; x++) {
          fb.set(x, y, RGB(4, 6, sky));
        }
      }
      // Stars
      for (int i = 0; i < 28; i++) {
        int sx = ((i * 53 + 17) % (W - 8)) + 4;
        int sy = ((i * 31 + 9) % (H / 2 + 10)) + 4;
        uint8_t tw = sin8(uint8_t(f * 7 + i * 29));
        uint8_t br = uint8_t(90 + (uint16_t(tw) * 165) / 255);
        fb.set(sx, sy, RGB(br, br, uint8_t(br > 220 ? 255 : br + 25)));
      }
      // Crescent moon
      int mx = (W * 3) / 4;
      int my = H / 4;
      int mr = H >= 100 ? 11 : 7;
      fb.fillCircleAA(mx, my, mr, RGB(255, 232, 165));
      fb.fillCircleAA(mx + mr / 2, my - mr / 4, mr, RGB(6, 10, 22));
      // Hills
      for (int x = 0; x < W; x++) {
        int h1 = (H * 3) / 4 + (int(sin8(uint8_t(x * 2))) - 128) / 14;
        int h2 = (H * 4) / 5 + (int(cos8(uint8_t(x * 3 + 40))) - 128) / 18;
        for (int y = h1; y < H; y++) fb.set(x, y, RGB(14, 34, 48));
        for (int y = h2; y < H; y++) fb.set(x, y, RGB(8, 22, 34));
      }
      return 65;
    }

    // Orbiting neon pixel rings on a deep cosmic backdrop
    int cx = W / 2;
    int cy = H / 2;
    int rad = (H < W ? H : W) / 3;
    for (int i = 0; i < 6; i++) {
      uint8_t a = uint8_t(f * 4 + i * 42);
      int ox = cx + ((int(cos8(a)) - 128) * rad) / 128;
      int oy = cy + ((int(sin8(a)) - 128) * rad) / 128;
      RGB c = palettes::byIndex(1).lookup(uint8_t(i * 42 + f * 2));
      fb.fillCircleAA(ox, oy, H >= 100 ? 6 : 4, c);
      int ox2 = cx + ((int(sin8(uint8_t(a + 64))) - 128) * (rad / 2)) / 128;
      int oy2 = cy + ((int(cos8(uint8_t(a * 2))) - 128) * (rad / 2)) / 128;
      RGB c2 = palettes::byIndex(2).lookup(uint8_t(255 - i * 42 - f * 2));
      fb.fillCircleAA(ox2, oy2, H >= 100 ? 3 : 2, c2);
    }
    return 45;
  }

 private:
  int clip_ = 0;
  uint32_t frame_ = 0;
};

struct WebApp {
  FrameBuffer fb;
  WebTime timeSource;
  WebInput heldKeys;
  WebAudio audio;
  Settings settings;
  WebSettingsStore store;
  DemoGifSource gifSource;
  WebEventSource eventSource;
  WebWeatherProvider weatherProvider;

  ClockScene clock;
  AnalogClockScene analogClock;
  WordClockScene wordClock;
  CalendarScene calendar;
  PongScene pong;
  SnakeScene snake;
  TetrisScene tetris;
  YogaScene yoga;
  ExerciseScene exercise;
  BreatheScene breathe;
  FocusScene focus;
  WeatherScene weather;
  GifScene gifs;
  SpiroScene spiro;
  MandalaScene mandala;
  CoriolisScene coriolisScene;
  LabyrinthScene labyrinth;
  LavaScene lava;
  RainScene rain;
  BounceScene bounce;
  StarfieldScene starfield;
  LifeScene life;
  AquariumScene aquarium;
  FireScene fire;
  AuroraScene aurora;
  SettingsScene settingsScene;

  Scene* scenes[25];
  Shell shell;
  Context ctx;

  uint8_t rgbaBuf[WIDTH * HEIGHT * 4];
  bool initialized = false;
  bool dirty = true;

  WebApp()
      : calendar(eventSource),
        pong(settings),
        snake(settings),
        tetris(settings),
        yoga(settings),
        exercise(settings),
        breathe(settings),
        weather(weatherProvider),
        gifs(gifSource),
        fire(settings),
        settingsScene(settings, store),
        scenes{&clock,     &analogClock, &wordClock,   &weather,
               &calendar,  &pong,        &snake,       &tetris,
               &labyrinth, &yoga,        &exercise,    &breathe,
               &focus,     &gifs,        &spiro,       &mandala,
               &coriolisScene, &lava,    &rain,        &bounce,
               &starfield, &life,        &aquarium,    &fire,
               &aurora},
        shell(scenes, 25, settings, settingsScene),
        ctx{fb, timeSource, heldKeys, audio, &palettes::byIndex(0), 0} {
    memset(rgbaBuf, 0, sizeof(rgbaBuf));
  }

  void blitRgba() {
    const FrameBuffer& out = shell.frame();
    uint8_t bright = shell.brightness(ctx);
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        RGB c = out.at(x, y);
        c.dim(bright);
        int dx = x, dy = y;
        switch (settings.rotation) {
          case 1:
            dx = HEIGHT - 1 - y;
            dy = x;
            break;
          case 2:
            dx = WIDTH - 1 - x;
            dy = HEIGHT - 1 - y;
            break;
          case 3:
            dx = y;
            dy = WIDTH - 1 - x;
            break;
        }
        int p = (dy * WIDTH + dx) * 4;
        rgbaBuf[p + 0] = c.r;
        rgbaBuf[p + 1] = c.g;
        rgbaBuf[p + 2] = c.b;
        rgbaBuf[p + 3] = 255;
      }
    }
  }
};

WebApp g_app;

Key keyFromId(int keyId) {
  switch (keyId) {
    case 1: return Key::Up;
    case 2: return Key::Down;
    case 3: return Key::Left;
    case 4: return Key::Right;
    case 5: return Key::Select;
    case 6: return Key::Back;
    default: return Key::None;
  }
}

Command cmdFromId(int cmdId) {
  switch (cmdId) {
    case 1: return Command::Menu;
    case 2: return Command::Settings;
    case 3: return Command::Overlay;
    case 4: return Command::Next;
    case 5: return Command::Home;
    default: return Command::None;
  }
}

}  // namespace

extern "C" {

WEB_EXPORT void coriolis_init(int showSplash) {
  if (!g_app.initialized) {
    g_app.store.load(g_app.settings);
    g_app.ctx.nowMs = 0;
    g_app.shell.begin(g_app.ctx, showSplash != 0);
    g_app.shell.tick(g_app.ctx);
    g_app.blitRgba();
    g_app.initialized = true;
    g_app.dirty = true;
  }
}

WEB_EXPORT void coriolis_set_time(int year, int month, int day, int hour,
                                  int minute, int second) {
  g_app.timeSource.set(year, month, day, hour, minute, second);
}

WEB_EXPORT int coriolis_tick(uint32_t nowMs) {
  if (!g_app.initialized) return 0;
  g_app.ctx.nowMs = nowMs;
  bool updated = g_app.shell.tick(g_app.ctx);
  if (updated || g_app.dirty) {
    g_app.blitRgba();
    g_app.dirty = false;
    return 1;
  }
  return 0;
}

WEB_EXPORT uint8_t* coriolis_rgba_ptr() { return g_app.rgbaBuf; }

WEB_EXPORT int coriolis_key(int keyId) {
  if (!g_app.initialized) return 0;
  bool handled = g_app.shell.key(g_app.ctx, keyFromId(keyId));
  g_app.dirty = true;
  return handled ? 1 : 0;
}

WEB_EXPORT void coriolis_cmd(int cmdId) {
  if (!g_app.initialized) return;
  g_app.shell.command(g_app.ctx, cmdFromId(cmdId));
  g_app.dirty = true;
}

WEB_EXPORT void coriolis_set_held(int keyId, int down) {
  g_app.heldKeys.setHeld(keyId, down != 0);
}

WEB_EXPORT void coriolis_show(int sceneIndex) {
  if (!g_app.initialized) return;
  g_app.shell.show(g_app.ctx, sceneIndex);
  g_app.dirty = true;
}

WEB_EXPORT void coriolis_rotate() {
  if (!g_app.initialized) return;
  if (WIDTH == HEIGHT) {
    g_app.settings.rotation = uint8_t((g_app.settings.rotation + 1) % 4);
    g_app.shell.toast().show("Rotated", g_app.ctx.nowMs);
    g_app.dirty = true;
  }
}

WEB_EXPORT int coriolis_toggle_night() {
  if (!g_app.initialized) return 0;
  g_app.settings.nightMode = !g_app.settings.nightMode;
  g_app.shell.toast().show(
      g_app.settings.nightMode ? "Night mode on" : "Night mode off",
      g_app.ctx.nowMs);
  g_app.dirty = true;
  return g_app.settings.nightMode ? 1 : 0;
}

WEB_EXPORT int coriolis_is_night() {
  if (!g_app.initialized) return 0;
  return g_app.shell.isNightActive(g_app.ctx) ? 1 : 0;
}

WEB_EXPORT void coriolis_cycle_weather() {
  if (!g_app.initialized) return;
  const char* msg = g_app.weatherProvider.cycle();
  g_app.shell.toast().show(msg, g_app.ctx.nowMs);
  g_app.dirty = true;
}

WEB_EXPORT int coriolis_scene_count() { return 25; }

WEB_EXPORT const char* coriolis_scene_name(int index) {
  if (index < 0 || index >= 25) return "";
  return g_app.scenes[index]->name();
}

WEB_EXPORT const char* coriolis_active_name() {
  return g_app.shell.activeName();
}

WEB_EXPORT const char* coriolis_palette_name() {
  return g_app.ctx.palette ? g_app.ctx.palette->name : "";
}

}  // extern "C"
