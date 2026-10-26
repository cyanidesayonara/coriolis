// Coriolis desktop simulator — the first display backend. Renders the
// framebuffer in a window with chunky pixels, feeds scenes the system clock,
// maps the keyboard to the input a remote/gamepad will provide, persists
// settings to a file, and serves GIFs from a ./gifs folder.
//
// Build: cmake -B build && cmake --build build   (see README.md)
#include "raylib.h"

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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>
#include <cmath>

using namespace coriolis;

// system clock — on hardware this becomes the Teensy RTC
class SystemTime : public TimeSource {
 public:
  TimeOfDay now() {
    time_t t = time(nullptr);
    struct tm* lt = localtime(&t);
    TimeOfDay tod;
    tod.hour = lt->tm_hour;
    tod.minute = lt->tm_min;
    tod.second = lt->tm_sec;
    tod.year = lt->tm_year + 1900;
    tod.month = lt->tm_mon + 1;
    tod.day = lt->tm_mday;
    return tod;
  }
};

// yearly events from events.txt next to the exe, one per line: "DD.MM LABEL"
// (on the device: the same file on the SD card)
class FileEventSource : public EventSource {
 public:
  int count() {
    if (!scanned_) scan();
    return count_;
  }
  const Event& get(int i) {
    if (!scanned_) scan();
    return events_[i < 0 ? 0 : (i >= count_ ? count_ - 1 : i)];
  }

 private:
  static const int MAXE = 48;
  Event events_[MAXE];
  int count_ = 0;
  bool scanned_ = false;

  void scan() {
    scanned_ = true;
    FILE* f = fopen("events.txt", "r");
    if (f) {
      char line[64];
      while (count_ < MAXE && fgets(line, sizeof(line), f)) {
        Event& e = events_[count_];
        if (sscanf(line, "%d.%d %19[^\r\n]", &e.day, &e.month, e.label) == 3 &&
            e.month >= 1 && e.month <= 12 && e.day >= 1 && e.day <= 31) {
          for (char* p = e.label; *p; ++p)
            *p = char(toupper((unsigned char)*p));
          count_++;
        }
      }
      fclose(f);
    }
    // the built-in local holidays ride along after the personal events
    for (int i = 0; i < BCN_HOLIDAY_COUNT && count_ < MAXE; i++)
      events_[count_++] = BCN_HOLIDAYS[i];
  }
};

// weather from weather.txt next to the exe: one line "COND TEMP", e.g.
// "rain 14" or "clear 27". Absent/malformed file -> the scene waits. On the
// device a tiny UART feeder module pushes the same two fields (or nothing).
class FileWeatherProvider : public WeatherProvider {
 public:
  WeatherInfo now() {
    WeatherInfo w;
    w.condition = Wx::Unknown;
    w.tempC = 0;
    w.valid = false;
    FILE* f = fopen("weather.txt", "r");
    if (!f) return w;
    char cond[16];
    int temp;
    if (fscanf(f, "%15s %d", cond, &temp) == 2) {
      w.condition = parse(cond);
      w.tempC = temp;
      w.valid = w.condition != Wx::Unknown;
    }
    fclose(f);
    return w;
  }

 private:
  static Wx parse(const char* s) {
    if (ieq(s, "clear") || ieq(s, "sun")) return Wx::Clear;
    if (ieq(s, "clouds") || ieq(s, "cloudy")) return Wx::Clouds;
    if (ieq(s, "rain")) return Wx::Rain;
    if (ieq(s, "snow")) return Wx::Snow;
    if (ieq(s, "storm")) return Wx::Storm;
    return Wx::Unknown;
  }
  static bool ieq(const char* a, const char* b) {
    for (; *a && *b; ++a, ++b)
      if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
    return *a == *b;
  }
};

// held keys for games — on hardware this becomes the gamepad
class KeyboardInput : public InputState {
 public:
  bool isDown(Key k) const {
    switch (k) {
      case Key::Up: return IsKeyDown(KEY_UP);
      case Key::Down: return IsKeyDown(KEY_DOWN);
      case Key::Left: return IsKeyDown(KEY_LEFT);
      case Key::Right: return IsKeyDown(KEY_RIGHT);
      case Key::Select: return IsKeyDown(KEY_ENTER);
      case Key::Back: return IsKeyDown(KEY_BACKSPACE);
      default: return false;
    }
  }
};

// audio cues as short synthesized beeps — the DFPlayer's stand-in. Off by
// default (toggle with A) so it doesn't chatter during development. On the
// device this becomes a DFPlayer Mini playing numbered clips from SD.
class SimAudio : public AudioSink {
 public:
  void init() {
    InitAudioDevice();
    ready_ = IsAudioDeviceReady();
    if (!ready_) return;
    make(Cue::Chime, 880, 90, false);
    make(Cue::StartBell, 660, 170, false);
    make(Cue::FinishBell, 523, 240, false);
    make(Cue::Halfway, 988, 110, false);
    make(Cue::Tick, 1175, 35, true);
    make(Cue::BreatheIn, 523, 220, false);
    make(Cue::BreatheHold, 440, 90, false);
    make(Cue::BreatheOut, 392, 280, false);
    make(Cue::Pop, 190, 55, true);
    make(Cue::Eat, 1046, 45, false);
    make(Cue::Die, 320, 340, true);
    make(Cue::Bounce, 640, 25, false);
    make(Cue::Score, 784, 140, false);
    voice_ = build(700, 70, false);
    voiceReady_ = true;
  }

  void setEnabled(bool e) { enabled_ = e; }
  bool enabled() const { return enabled_; }
  bool ready() const { return ready_; }

  void play(Cue c) {
    int i = int(c);
    if (ready_ && enabled_ && i > 0 && i < NCUE && loaded_[i]) PlaySound(snd_[i]);
  }
  void voice(int) { if (ready_ && enabled_ && voiceReady_) PlaySound(voice_); }
  void loop(Cue) {}  // the ambient crackle bed isn't modeled in the sim
  void setVolume(uint8_t v) {
    if (ready_) SetMasterVolume(v / 30.0f);
  }

 private:
  static const int NCUE = 20;
  Sound snd_[NCUE];
  bool loaded_[NCUE] = {false};
  Sound voice_;
  bool ready_ = false, enabled_ = false, voiceReady_ = false;

  void make(Cue c, float f, int ms, bool descend) {
    int i = int(c);
    snd_[i] = build(f, ms, descend);
    loaded_[i] = true;
  }

  Sound build(float f, int ms, bool descend) {
    unsigned int sr = 22050, n = sr * unsigned(ms) / 1000;
    short* d = (short*)malloc(size_t(n) * 2);
    for (unsigned i = 0; i < n; i++) {
      float prog = float(i) / n;
      float freq = descend ? f * (1.0f - 0.6f * prog) : f;
      float env = 1.0f - prog;  // linear decay
      float s = sinf(2.0f * 3.14159265f * freq * (float(i) / sr)) * env * 0.35f;
      d[i] = short(s * 32767);
    }
    Wave w;
    w.frameCount = n;
    w.sampleRate = sr;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = d;
    Sound snd = LoadSoundFromWave(w);
    free(d);
    return snd;
  }
};

// settings in a key=value text file next to the exe — on hardware this
// becomes SD or emulated EEPROM
class FileSettingsStore : public SettingsStore {
 public:
  bool load(Settings& out) {
    FILE* f = fopen(FILENAME, "r");
    if (!f) return false;
    char key[32];
    int value;
    while (fscanf(f, "%31[^=]=%d\n", key, &value) == 2) {
      if (strcmp(key, "brightness") == 0) out.brightness = uint8_t(value);
      else if (strcmp(key, "palette") == 0) out.paletteIndex = value;
      else if (strcmp(key, "rotation") == 0) out.rotation = uint8_t(value);
      else if (strcmp(key, "autoplay") == 0) out.autoplay = value != 0;
      else if (strcmp(key, "autoplaySeconds") == 0)
        out.autoplaySeconds = uint16_t(value);
      else if (strcmp(key, "transitionMode") == 0)
        out.transitionMode = uint8_t(value);
      else if (strcmp(key, "volume") == 0) out.volume = uint8_t(value);
      else if (strcmp(key, "zoneHours") == 0) out.zoneHours = int8_t(value);
      else if (strcmp(key, "euDst") == 0) out.euDst = value != 0;
      else if (strcmp(key, "yogaBody") == 0) out.yogaBody = uint8_t(value);
      else if (strcmp(key, "yogaProgram") == 0)
        out.yogaProgram = uint8_t(value);
      else if (strcmp(key, "yogaHoldSec") == 0)
        out.yogaHoldSec = uint16_t(value);
      else if (strcmp(key, "exerciseProgram") == 0)
        out.exerciseProgram = uint8_t(value);
      else if (strcmp(key, "exerciseReps") == 0)
        out.exerciseReps = uint8_t(value);
      else if (strcmp(key, "exerciseRepSec") == 0)
        out.exerciseRepSec = uint8_t(value);
      else if (strcmp(key, "breatheStyle") == 0)
        out.breatheStyle = uint8_t(value);
      else if (strcmp(key, "breatheSec") == 0) out.breatheSec = uint8_t(value);
      else if (strcmp(key, "pongLevel") == 0) out.pongLevel = uint8_t(value);
      else if (strcmp(key, "snakeSpeed") == 0) out.snakeSpeed = uint8_t(value);
      else if (strcmp(key, "overlayType") == 0)
        out.overlayType = uint8_t(value);
      else if (strcmp(key, "overlayPos") == 0) out.overlayPos = uint8_t(value);
      else if (strcmp(key, "overlaySize") == 0)
        out.overlaySize = uint8_t(value);
      else if (strcmp(key, "fireSparks") == 0) out.fireSparks = value != 0;
      else if (strcmp(key, "nightMode") == 0) out.nightMode = value != 0;
      else if (strcmp(key, "nightStartHour") == 0) out.nightStartHour = uint8_t(value);
      else if (strcmp(key, "nightEndHour") == 0) out.nightEndHour = uint8_t(value);
      else if (strcmp(key, "nightBrightness") == 0) out.nightBrightness = uint8_t(value);
      else if (strcmp(key, "snakeHigh") == 0) out.snakeHigh = uint16_t(value);
      else if (strcmp(key, "tetrisHigh") == 0)
        out.tetrisHigh = uint32_t(value);
    }
    fclose(f);
    return true;
  }

  void save(const Settings& s) {
    FILE* f = fopen(FILENAME, "w");
    if (!f) return;
    fprintf(f, "brightness=%d\n", s.brightness);
    fprintf(f, "palette=%d\n", s.paletteIndex);
    fprintf(f, "rotation=%d\n", s.rotation);
    fprintf(f, "autoplay=%d\n", s.autoplay ? 1 : 0);
    fprintf(f, "autoplaySeconds=%d\n", s.autoplaySeconds);
    fprintf(f, "transitionMode=%d\n", s.transitionMode);
    fprintf(f, "volume=%d\n", s.volume);
    fprintf(f, "zoneHours=%d\n", s.zoneHours);
    fprintf(f, "euDst=%d\n", s.euDst ? 1 : 0);
    fprintf(f, "yogaBody=%d\n", s.yogaBody);
    fprintf(f, "yogaProgram=%d\n", s.yogaProgram);
    fprintf(f, "yogaHoldSec=%d\n", s.yogaHoldSec);
    fprintf(f, "exerciseProgram=%d\n", s.exerciseProgram);
    fprintf(f, "exerciseReps=%d\n", s.exerciseReps);
    fprintf(f, "exerciseRepSec=%d\n", s.exerciseRepSec);
    fprintf(f, "breatheStyle=%d\n", s.breatheStyle);
    fprintf(f, "breatheSec=%d\n", s.breatheSec);
    fprintf(f, "pongLevel=%d\n", s.pongLevel);
    fprintf(f, "snakeSpeed=%d\n", s.snakeSpeed);
    fprintf(f, "overlayType=%d\n", s.overlayType);
    fprintf(f, "overlayPos=%d\n", s.overlayPos);
    fprintf(f, "overlaySize=%d\n", s.overlaySize);
    fprintf(f, "fireSparks=%d\n", s.fireSparks ? 1 : 0);
    fprintf(f, "nightMode=%d\n", s.nightMode ? 1 : 0);
    fprintf(f, "nightStartHour=%d\n", s.nightStartHour);
    fprintf(f, "nightEndHour=%d\n", s.nightEndHour);
    fprintf(f, "nightBrightness=%d\n", s.nightBrightness);
    fprintf(f, "snakeHigh=%d\n", s.snakeHigh);
    fprintf(f, "tetrisHigh=%d\n", int(s.tetrisHigh));
    fclose(f);
  }

 private:
  static const char* const FILENAME;
};
const char* const FileSettingsStore::FILENAME = "coriolis_settings.txt";

// GIFs from ./gifs via raylib's animated GIF loader — on hardware this
// becomes pixelmatix/GifDecoder reading from SD
class RaylibGifSource : public GifSource {
 public:
  ~RaylibGifSource() {
    if (img_.data) UnloadImage(img_);
    if (scanned_ && files_.count > 0) UnloadDirectoryFiles(files_);
  }

  int count() {
    if (!scanned_) {
      scanned_ = true;
      if (DirectoryExists("gifs"))
        files_ = LoadDirectoryFilesEx("gifs", ".gif", false);
    }
    return int(files_.count);
  }

  bool open(int index) {
    if (img_.data) { UnloadImage(img_); img_.data = nullptr; }
    frames_ = 0;
    frame_ = 0;
    if (index < 0 || index >= count()) return false;
    img_ = LoadImageAnim(files_.paths[index], &frames_);
    return img_.data != nullptr && frames_ > 0;
  }

  uint32_t drawNextFrame(FrameBuffer& fb) {
    if (!img_.data || frames_ < 1) return 0;

    // nearest-neighbor fit, centered, aspect preserved
    float sx = float(fb.width()) / img_.width;
    float sy = float(fb.height()) / img_.height;
    float s = sx < sy ? sx : sy;
    int dw = int(img_.width * s);
    int dh = int(img_.height * s);
    int ox = (fb.width() - dw) / 2;
    int oy = (fb.height() - dh) / 2;

    const Color* pixels =
        reinterpret_cast<const Color*>(img_.data) +
        size_t(frame_) * img_.width * img_.height;

    fb.clear();
    for (int y = 0; y < dh; y++) {
      int srcY = int(y / s);
      for (int x = 0; x < dw; x++) {
        const Color& c = pixels[srcY * img_.width + int(x / s)];
        fb.set(ox + x, oy + y, RGB(c.r, c.g, c.b));
      }
    }

    frame_ = (frame_ + 1) % frames_;
    return 80;  // raylib doesn't expose per-frame delays; 80ms reads well
  }

 private:
  FilePathList files_ = {0, 0, nullptr};
  bool scanned_ = false;
  Image img_ = {nullptr, 0, 0, 0, 0};
  int frames_ = 0;
  int frame_ = 0;
};

// write the framebuffer to a PNG, scaled up with chunky pixels and a thin
// grid, so it reads like the real panel in the README
static void exportShot(const FrameBuffer& fb, const char* path, int scale) {
  int W = fb.width() * scale, H = fb.height() * scale;
  unsigned char* buf = (unsigned char*)malloc(size_t(W) * H * 3);
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      RGB c = fb.at(x / scale, y / scale);
      bool grid = (x % scale == 0) || (y % scale == 0);
      if (grid) c.dim(150);
      int o = (y * W + x) * 3;
      buf[o] = c.r; buf[o + 1] = c.g; buf[o + 2] = c.b;
    }
  }
  Image img = {buf, W, H, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8};
  ExportImage(img, path);
  free(buf);
}

static void writeShot(const char* dir, const char* file, const FrameBuffer& fb) {
  char path[256];
  snprintf(path, sizeof(path), "%s/%s", dir, file);
  exportShot(fb, path, 5);
  printf("wrote %s\n", path);
}

// render one scene to a PNG: pick a palette, press a few keys (OK starts a
// guide/game, arrows skip ahead), let it run, and stamp on the overlay if
// asked. Time is simulated at 60 fps.
static void shoot(const char* dir, const char* file, Scene* sc,
                  Settings settings, TimeSource& time, InputState& held,
                  AudioSink& audio, int paletteIdx, bool startIt, int frames,
                  const Key* keys = nullptr, int keyCount = 0,
                  int keyEvery = 0) {
  FrameBuffer fb;
  settings.paletteIndex = paletteIdx;
  Context ctx = {fb, time, held, audio, &palettes::byIndex(paletteIdx), 0};
  sc->start(ctx);
  if (startIt) sc->input(ctx, Key::Select);
  int k = 0;
  for (int f = 0; f < frames; f++) {
    ctx.nowMs = uint32_t(f * 16);
    ctx.palette = &palettes::byIndex(paletteIdx);
    if (keys && k < keyCount && keyEvery > 0 && f > 0 && f % keyEvery == 0)
      sc->input(ctx, keys[k++]);
    sc->draw(ctx);
    if (settings.overlayType) overlay::draw(ctx, settings);
  }
  writeShot(dir, file, fb);
}

// fixed demo events so the gallery's calendar shot shows a countdown
class DemoEvents : public EventSource {
 public:
  int count() { return 2; }
  const Event& get(int i) {
    static const Event e[2] = {{24, 12, "XMAS"}, {31, 12, "NEW YEAR"}};
    return e[i == 1 ? 1 : 0];
  }
};

// a fixed sunny 27C so the gallery's weather shot has something to show
class DemoWeather : public WeatherProvider {
 public:
  WeatherInfo now() { return {Wx::Clear, 27, true}; }
};

// a settings store that never touches disk, for the gallery
class NullStore : public SettingsStore {
 public:
  bool load(Settings&) { return false; }
  void save(const Settings&) {}
};

// the PC clock, but claiming to be settable like the device RTC, so the
// settings shot shows real values in its Clock section
class DemoDeviceTime : public SystemTime {
 public:
  bool settable() const { return true; }
};

// shots of the shell itself (splash, launcher, settings): drive a real
// Shell with simulated time and keys
static void shootShell(const char* dir, TimeSource& time, InputState& held,
                       AudioSink& audio) {
  Settings s;
  NullStore store;
  FrameBuffer fb;
  ClockScene clock;
  YogaScene yoga(s);
  FireScene fire(s);
  AuroraScene aurora;
  SettingsScene settingsScene(s, store);
  Scene* list[] = {&clock, &yoga, &aurora, &fire};
  Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 0};

  {  // the splash, mid-resolve
    Shell shell(list, 4, s, settingsScene);
    shell.begin(ctx, true);
    for (int f = 0; f < 120; f++) { ctx.nowMs = uint32_t(f * 16); shell.tick(ctx); }
    writeShot(dir, "splash.png", shell.frame());
  }
  {  // the launcher over a dimmed aurora, browsing ambient art
    Shell shell(list, 4, s, settingsScene);
    ctx.nowMs = 0;
    shell.begin(ctx, false);
    shell.show(ctx, 2);
    for (int f = 0; f < 200; f++) { ctx.nowMs = uint32_t(f * 16); shell.tick(ctx); }
    shell.command(ctx, Command::Menu);
    shell.key(ctx, Key::Down);
    for (int f = 200; f < 240; f++) { ctx.nowMs = uint32_t(f * 16); shell.tick(ctx); }
    writeShot(dir, "launcher.png", shell.frame());
  }
  {  // settings, opened from the clock (its CLOCK section)
    DemoDeviceTime deviceTime;
    Context dctx = {fb, deviceTime, held, audio, &palettes::byIndex(0), 0};
    Context& ctx = dctx;
    Shell shell(list, 4, s, settingsScene);
    ctx.nowMs = 0;
    shell.begin(ctx, false);
    shell.command(ctx, Command::Settings);
    for (int f = 0; f < 20; f++) { ctx.nowMs = uint32_t(f * 16); shell.tick(ctx); }
    writeShot(dir, "settings.png", shell.frame());
  }
}

// `coriolis_sim --shots <dir>` renders the gallery and exits
static int renderShots(const char* dir) {
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(64, 64, "shots");  // a context is needed for image encoding

  SystemTime time;
  KeyboardInput held;
  SilentAudio audio;
  Settings s;              // defaults; per-shot overrides below
  Settings ov = s;
  ov.overlayType = 1;      // digital overlay, centered
  ov.overlaySize = 2;

  ClockScene clock;               shoot(dir, "clock.png", &clock, s, time, held, audio, 2, false, 60);
  AnalogClockScene analog;        shoot(dir, "analog.png", &analog, s, time, held, audio, 2, false, 60);
  WordClockScene word;            shoot(dir, "wordclock.png", &word, s, time, held, audio, 3, false, 60);
  DemoEvents demoEvents;
  CalendarScene cal(demoEvents);  shoot(dir, "calendar.png", &cal, s, time, held, audio, 0, false, 30);
  SpiroScene spiro;               shoot(dir, "spiro.png", &spiro, s, time, held, audio, 0, false, 900);
  MandalaScene mandala;           shoot(dir, "mandala.png", &mandala, s, time, held, audio, 4, false, 500);
  CoriolisScene corio;            shoot(dir, "coriolis.png", &corio, s, time, held, audio, 0, false, 420);
  LabyrinthScene labyrinth;       shoot(dir, "labyrinth.png", &labyrinth, s, time, held, audio, 0, true, 40);
  LavaScene lava;                 shoot(dir, "lava.png", &lava, s, time, held, audio, 1, false, 260);
  RainScene rain;                 shoot(dir, "rain.png", &rain, s, time, held, audio, 6, false, 200);
  FireScene fire(s);              shoot(dir, "fireplace.png", &fire, s, time, held, audio, 0, false, 300);
  AuroraScene auroraShot;         shoot(dir, "aurora.png", &auroraShot, s, time, held, audio, 3, false, 220);

  // yoga: the standing flow, skipped ahead to warrior 1 on the LEFT side
  Settings ys = s; ys.yogaProgram = 2;
  const Key skip2[] = {Key::Right, Key::Right};
  YogaScene yoga(ys);             shoot(dir, "yoga.png", &yoga, ys, time, held, audio, 0, true, 420, skip2, 2, 200);
  YogaScene yogaCard(ys);         shoot(dir, "yoga_setup.png", &yogaCard, ys, time, held, audio, 0, false, 40);
  // the Namaste card: skip through every step
  Key skipAll[20];
  for (int i = 0; i < 20; i++) skipAll[i] = Key::Right;
  YogaScene yogaDone(ys);         shoot(dir, "yoga_done.png", &yogaDone, ys, time, held, audio, 0, true, 900, skipAll, 20, 40);

  Settings kb = s; kb.exerciseProgram = 1;
  ExerciseScene ex(kb);           shoot(dir, "exercise.png", &ex, kb, time, held, audio, 0, true, 300);
  // the rest screen between moves, previewing the goblet squat
  const Key oneSkip[] = {Key::Right};
  ExerciseScene exRest(kb);       shoot(dir, "exercise_rest.png", &exRest, kb, time, held, audio, 0, true, 300, oneSkip, 1, 230);
  Settings tb = s; tb.exerciseProgram = 2;
  ExerciseScene tabata(tb);       shoot(dir, "tabata.png", &tabata, tb, time, held, audio, 0, true, 420);
  ExerciseScene countdown(tb);    shoot(dir, "countdown.png", &countdown, tb, time, held, audio, 0, true, 40);

  BreatheScene breathe(s);        shoot(dir, "breathe.png", &breathe, s, time, held, audio, 0, true, 60);
  PongScene pong(s);              shoot(dir, "pong.png", &pong, s, time, held, audio, 0, true, 120);
  SnakeScene snake(s);            shoot(dir, "snake.png", &snake, s, time, held, audio, 0, true, 110);
  TetrisScene tetris(s);          shoot(dir, "tetris.png", &tetris, s, time, held, audio, 0, true, 500);
  FocusScene focus;               shoot(dir, "focus.png", &focus, s, time, held, audio, 0, true, 60);
  DemoWeather demoWx;
  WeatherScene weather(demoWx);   shoot(dir, "weather.png", &weather, s, time, held, audio, 0, false, 120);
  BounceScene bounce;             shoot(dir, "bounce.png", &bounce, s, time, held, audio, 4, false, 260);
  StarfieldScene starfield;       shoot(dir, "starfield.png", &starfield, s, time, held, audio, 6, false, 200);
  LifeScene life;                 shoot(dir, "life.png", &life, s, time, held, audio, 3, false, 120);
  AquariumScene aquarium;         shoot(dir, "aquarium.png", &aquarium, s, time, held, audio, 2, false, 200);
  SpiroScene spiro2;              shoot(dir, "overlay.png", &spiro2, ov, time, held, audio, 0, false, 900);

  shootShell(dir, time, held, audio);

  CloseWindow();
  return 0;
}

// `coriolis_sim --poses <dir>`: every step of every yoga flow and exercise
// program, settled, for checking figure work at a glance
static int renderPoses(const char* dir) {
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(64, 64, "poses");
  SystemTime time;
  KeyboardInput held;
  SilentAudio audio;
  FrameBuffer fb;
  char name[64];
  for (int p = 0; p < YogaScene::FLOW_COUNT; p++) {
    Settings s;
    s.yogaProgram = uint8_t(p);
    s.yogaHoldSec = 60;
    YogaScene yoga(s);
    Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 0};
    yoga.start(ctx);
    yoga.input(ctx, Key::Select);
    uint32_t t = 0;
    for (; t < 3300; t += 16) { ctx.nowMs = t; yoga.draw(ctx); }
    for (int step = 0; step < 16; step++) {
      for (uint32_t e = 0; e < 1600; e += 16) { ctx.nowMs = t += 16; yoga.draw(ctx); }
      snprintf(name, sizeof(name), "yoga%d_%02d.png", p, step);
      writeShot(dir, name, fb);
      yoga.input(ctx, Key::Right);
      ctx.nowMs = t += 16;
      yoga.draw(ctx);
    }
  }
  for (int p = 0; p < ExerciseScene::PROGRAM_COUNT; p++) {
    Settings s;
    s.exerciseProgram = uint8_t(p);
    ExerciseScene ex(s);
    Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 0};
    ex.start(ctx);
    ex.input(ctx, Key::Select);
    uint32_t t = 0;
    for (; t < 3300; t += 16) { ctx.nowMs = t; ex.draw(ctx); }
    for (int step = 0; step < 18; step++) {
      for (uint32_t e = 0; e < 1450; e += 16) { ctx.nowMs = t += 16; ex.draw(ctx); }
      snprintf(name, sizeof(name), "ex%d_%02d.png", p, step);
      writeShot(dir, name, fb);
      ex.input(ctx, Key::Right);
    }
  }
  CloseWindow();
  return 0;
}

int main(int argc, char** argv) {
  if (argc >= 3 && strcmp(argv[1], "--shots") == 0)
    return renderShots(argv[2]);
  if (argc >= 3 && strcmp(argv[1], "--poses") == 0)
    return renderPoses(argv[2]);
  bool splash = !(argc >= 2 && strcmp(argv[1], "--no-splash") == 0);

  // chunky pixels, the whole point — but keep the window on a 1080p screen
  int scale = 8;
  while (scale > 2 && (WIDTH * scale > 1700 || HEIGHT * scale > 900)) scale--;
  InitWindow(WIDTH * scale, HEIGHT * scale, "Coriolis");
  SetTargetFPS(120);

  FrameBuffer fb;
  SystemTime timeSource;
  KeyboardInput heldKeys;
  SimAudio audio;
  audio.init();
  printf("audio device: %s (press A in the window to toggle sound)\n",
         audio.ready() ? "ready" : "NOT available");

  Settings settings;
  FileSettingsStore store;
  store.load(settings);

  RaylibGifSource gifSource;

  FileEventSource eventSource;
  FileWeatherProvider weatherProvider;

  ClockScene clock;
  AnalogClockScene analogClock;
  WordClockScene wordClock;
  CalendarScene calendar(eventSource);
  PongScene pong(settings);
  SnakeScene snake(settings);
  TetrisScene tetris(settings);
  YogaScene yoga(settings);
  ExerciseScene exercise(settings);
  BreatheScene breathe(settings);
  FocusScene focus;
  WeatherScene weather(weatherProvider);
  GifScene gifs(gifSource);
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
  FireScene fire(settings);
  AuroraScene aurora;
  SettingsScene settingsScene(settings, store);

  // settings is deliberately NOT in the rotation: it opens on its own key,
  // so it can't be stumbled into or autoplayed through
  Scene* scenes[] = {&clock,     &analogClock, &wordClock,   &weather,
                     &calendar,  &pong,        &snake,       &tetris,
                     &labyrinth, &yoga,        &exercise,    &breathe,
                     &focus,     &gifs,        &spiro,       &mandala,
                     &coriolisScene, &lava,    &rain,        &bounce,
                     &starfield, &life,        &aquarium,    &fire,
                     &aurora};
  const int sceneCount = sizeof(scenes) / sizeof(scenes[0]);

  Context ctx = {fb, timeSource, heldKeys, audio,
                 &palettes::byIndex(settings.paletteIndex), 0};
  Shell shell(scenes, sceneCount, settings, settingsScene);
  shell.begin(ctx, splash);

  while (!WindowShouldClose()) {
    ctx.nowMs = uint32_t(GetTime() * 1000.0);

    // keyboard stands in for the remote: arrows/enter/backspace are the
    // navigation keys, the letters are the remote's extra buttons
    if (IsKeyPressed(KEY_UP)) shell.key(ctx, Key::Up);
    if (IsKeyPressed(KEY_DOWN)) shell.key(ctx, Key::Down);
    if (IsKeyPressed(KEY_LEFT)) shell.key(ctx, Key::Left);
    if (IsKeyPressed(KEY_RIGHT)) shell.key(ctx, Key::Right);
    if (IsKeyPressed(KEY_ENTER)) shell.key(ctx, Key::Select);
    if (IsKeyPressed(KEY_BACKSPACE)) shell.key(ctx, Key::Back);
    if (IsKeyPressed(KEY_M)) shell.command(ctx, Command::Menu);
    if (IsKeyPressed(KEY_S)) shell.command(ctx, Command::Settings);
    if (IsKeyPressed(KEY_C)) shell.command(ctx, Command::Overlay);
    if (IsKeyPressed(KEY_SPACE)) shell.command(ctx, Command::Next);
    if (IsKeyPressed(KEY_H)) shell.command(ctx, Command::Home);

    if (IsKeyPressed(KEY_R) && WIDTH == HEIGHT) {
      settings.rotation = uint8_t((settings.rotation + 1) % 4);
      shell.toast().show("Rotated", ctx.nowMs);
    }

    // A toggles the simulator's beep audio (off by default)
    if (IsKeyPressed(KEY_A)) {
      audio.setEnabled(!audio.enabled());
      shell.toast().show(audio.enabled() ? "Audio on" : "Audio off", ctx.nowMs);
    }

    shell.tick(ctx);

    BeginDrawing();
    ClearBackground(BLACK);
    const FrameBuffer& out = shell.frame();
    uint8_t bright = shell.brightness(ctx);
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        RGB c = out.at(x, y);
        c.dim(bright);
        int dx = x, dy = y;
        switch (settings.rotation) {
          case 1: dx = HEIGHT - 1 - y; dy = x; break;
          case 2: dx = WIDTH - 1 - x; dy = HEIGHT - 1 - y; break;
          case 3: dx = y; dy = WIDTH - 1 - x; break;
        }
        DrawRectangle(dx * scale, dy * scale, scale - 1, scale - 1,
                      Color{c.r, c.g, c.b, 255});
      }
    }
    // debug labels at the bottom, clear of the matrix's own top bar
    DrawText(shell.activeName(), 8, HEIGHT * scale - 42, 20, RAYWHITE);
    DrawText(ctx.palette->name, 8, HEIGHT * scale - 20, 14, GRAY);
    const char* astat = !audio.ready() ? "audio: n/a"
                                        : (audio.enabled() ? "audio: ON [A]"
                                                           : "audio: off [A]");
    DrawText(astat, WIDTH * scale - 130, HEIGHT * scale - 20, 14,
             audio.enabled() ? GREEN : GRAY);
    EndDrawing();
  }

  store.save(settings);
  CloseWindow();
  return 0;
}
