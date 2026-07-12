// Coriolis desktop simulator — the first display backend. Renders the
// framebuffer in a window with chunky pixels, feeds scenes the system clock,
// maps the keyboard to the input a remote/gamepad will provide, persists
// settings to a file, and serves GIFs from a ./gifs folder.
//
// Build: cmake -B build && cmake --build build   (see README.md)
#include "raylib.h"

#include "../core/config.h"
#include "../core/display.h"
#include "../core/scene.h"
#include "../core/settings.h"
#include "../scenes/scene_clock.h"
#include "../scenes/scene_analogclock.h"
#include "../scenes/scene_wordclock.h"
#include "../scenes/scene_pong.h"
#include "../scenes/scene_snake.h"
#include "../scenes/scene_tetris.h"
#include "../scenes/scene_yoga.h"
#include "../scenes/scene_exercise.h"
#include "../scenes/scene_breathe.h"
#include "../scenes/scene_gifs.h"
#include "../scenes/scene_plasma.h"
#include "../scenes/scene_fire.h"
#include "../scenes/scene_spiro.h"
#include "../scenes/scene_mandala.h"
#include "../scenes/scene_rain.h"
#include "../scenes/scene_bounce.h"
#include "../scenes/scene_starfield.h"
#include "../scenes/scene_life.h"
#include "../scenes/scene_aquarium.h"
#include "../scenes/scene_settings.h"
#include "../scenes/clock_overlay.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
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
    return tod;
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

 private:
  static const int NCUE = 16;
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
      else if (strcmp(key, "yogaBody") == 0) out.yogaBody = uint8_t(value);
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
    fprintf(f, "yogaBody=%d\n", s.yogaBody);
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
static void exportShot(FrameBuffer& fb, const char* path, int scale) {
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

// render one scene to a PNG: pick a palette, optionally press OK to start a
// guide/game, let it settle, and stamp on the overlay if asked
static void shoot(const char* dir, const char* file, Scene* sc,
                  Settings settings, TimeSource& time, InputState& held,
                  AudioSink& audio, int paletteIdx, bool startIt, int frames) {
  FrameBuffer fb;
  settings.paletteIndex = paletteIdx;
  Context ctx = {fb, time, held, audio, &palettes::byIndex(paletteIdx), 0};
  sc->start(ctx);
  if (startIt) sc->input(ctx, Key::Select);
  for (int f = 0; f < frames; f++) {
    ctx.nowMs = uint32_t(f * 16);
    ctx.palette = &palettes::byIndex(paletteIdx);
    sc->draw(ctx);
    if (settings.overlayType) overlay::draw(ctx, settings);
  }
  char path[256];
  snprintf(path, sizeof(path), "%s/%s", dir, file);
  exportShot(fb, path, 5);
  printf("wrote %s\n", path);
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
  SpiroScene spiro;               shoot(dir, "spiro.png", &spiro, s, time, held, audio, 0, false, 900);
  MandalaScene mandala;           shoot(dir, "mandala.png", &mandala, s, time, held, audio, 4, false, 500);
  RainScene rain;                 shoot(dir, "rain.png", &rain, s, time, held, audio, 6, false, 200);
  FireScene fire(s);              shoot(dir, "fireplace.png", &fire, s, time, held, audio, 0, false, 300);
  PlasmaScene plasma;             shoot(dir, "plasma.png", &plasma, s, time, held, audio, 0, false, 60);
  YogaScene yoga(s);              shoot(dir, "yoga.png", &yoga, s, time, held, audio, 0, true, 120);
  Settings kb = s; kb.exerciseProgram = 1;
  ExerciseScene ex(kb);           shoot(dir, "exercise.png", &ex, kb, time, held, audio, 0, true, 90);
  BreatheScene breathe(s);        shoot(dir, "breathe.png", &breathe, s, time, held, audio, 0, true, 60);
  PongScene pong(s);              shoot(dir, "pong.png", &pong, s, time, held, audio, 0, true, 120);
  SnakeScene snake(s);            shoot(dir, "snake.png", &snake, s, time, held, audio, 0, true, 200);
  TetrisScene tetris;             shoot(dir, "tetris.png", &tetris, s, time, held, audio, 0, true, 500);
  BounceScene bounce;             shoot(dir, "bounce.png", &bounce, s, time, held, audio, 4, false, 260);
  StarfieldScene starfield;       shoot(dir, "starfield.png", &starfield, s, time, held, audio, 6, false, 200);
  LifeScene life;                 shoot(dir, "life.png", &life, s, time, held, audio, 3, false, 120);
  AquariumScene aquarium;         shoot(dir, "aquarium.png", &aquarium, s, time, held, audio, 2, false, 200);
  SpiroScene spiro2;              shoot(dir, "overlay.png", &spiro2, ov, time, held, audio, 0, false, 900);

  CloseWindow();
  return 0;
}

int main(int argc, char** argv) {
  if (argc >= 3 && strcmp(argv[1], "--shots") == 0)
    return renderShots(argv[2]);

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

  ClockScene clock;
  AnalogClockScene analogClock;
  WordClockScene wordClock;
  PongScene pong(settings);
  SnakeScene snake(settings);
  TetrisScene tetris;
  YogaScene yoga(settings);
  ExerciseScene exercise(settings);
  BreatheScene breathe(settings);
  GifScene gifs(gifSource);
  SpiroScene spiro;
  MandalaScene mandala;
  RainScene rain;
  BounceScene bounce;
  StarfieldScene starfield;
  LifeScene life;
  AquariumScene aquarium;
  FireScene fire(settings);
  PlasmaScene plasma;
  SettingsScene settingsScene(settings, store);

  // settings is deliberately NOT in the rotation: it opens on its own key,
  // so it can't be stumbled into or autoplayed through
  Scene* scenes[] = {&clock,     &analogClock, &wordClock, &pong,
                     &snake,     &tetris,      &yoga,      &exercise,
                     &breathe,   &gifs,        &spiro,     &mandala,
                     &rain,      &bounce,      &starfield, &life,
                     &aquarium,  &fire,        &plasma};
  const int sceneCount = sizeof(scenes) / sizeof(scenes[0]);
  int current = 0;
  bool inSettings = false;

  Context ctx = {fb, timeSource, heldKeys, audio,
                 &palettes::byIndex(settings.paletteIndex), 0};

  scenes[current]->start(ctx);

  uint32_t nextFrameMs = 0;
  uint32_t lastSwitchMs = 0;

  while (!WindowShouldClose()) {
    ctx.nowMs = uint32_t(GetTime() * 1000.0);
    ctx.palette = &palettes::byIndex(settings.paletteIndex);

    // keyboard stands in for the remote/gamepad; scenes get first refusal
    Key pressed = Key::None;
    if (IsKeyPressed(KEY_UP)) pressed = Key::Up;
    if (IsKeyPressed(KEY_DOWN)) pressed = Key::Down;
    if (IsKeyPressed(KEY_LEFT)) pressed = Key::Left;
    if (IsKeyPressed(KEY_RIGHT)) pressed = Key::Right;
    if (IsKeyPressed(KEY_ENTER)) pressed = Key::Select;
    if (IsKeyPressed(KEY_BACKSPACE)) pressed = Key::Back;

    Scene* active =
        inSettings ? static_cast<Scene*>(&settingsScene) : scenes[current];

    bool consumed = false;
    if (pressed != Key::None) consumed = active->input(ctx, pressed);

    // the settings key toggles the menu from anywhere and jumps to the
    // section of the scene it was opened from (device: the remote's menu
    // button)
    if (IsKeyPressed(KEY_S)) {
      if (inSettings) {
        settingsScene.stop(ctx);  // persists any changes
        inSettings = false;
      } else {
        settingsScene.openSection(scenes[current]->name());
        inSettings = true;
      }
      nextFrameMs = 0;
      consumed = true;
    }

    int switchTo = -1;
    if (!consumed && inSettings) {
      if (pressed == Key::Back) {  // back also closes the menu
        settingsScene.stop(ctx);
        inSettings = false;
        nextFrameMs = 0;
      }
    } else if (!consumed) {
      if (pressed == Key::Right || IsKeyPressed(KEY_SPACE))
        switchTo = (current + 1) % sceneCount;
      else if (pressed == Key::Left)
        switchTo = (current + sceneCount - 1) % sceneCount;
      else if (pressed == Key::Up)
        settings.paletteIndex =
            (settings.paletteIndex + 1) % palettes::COUNT;
      else if (pressed == Key::Down)
        settings.paletteIndex =
            (settings.paletteIndex + palettes::COUNT - 1) % palettes::COUNT;
      else if (pressed == Key::Back)
        switchTo = 0;  // home to the clock from anywhere
    }

    if (IsKeyPressed(KEY_R) && WIDTH == HEIGHT)
      settings.rotation = uint8_t((settings.rotation + 1) % 4);

    // the clock-overlay button: cycle off -> digital -> analog -> word
    if (IsKeyPressed(KEY_C))
      settings.overlayType = uint8_t((settings.overlayType + 1) % 4);

    // A toggles the simulator's beep audio (off by default)
    if (IsKeyPressed(KEY_A)) audio.setEnabled(!audio.enabled());

    // autoplay cycles eligible scenes; paused while the menu is open
    if (!inSettings && settings.autoplay && switchTo < 0 &&
        scenes[current]->autoplayEligible() &&
        ctx.nowMs - lastSwitchMs >= uint32_t(settings.autoplaySeconds) * 1000) {
      switchTo = (current + 1) % sceneCount;
      while (!scenes[switchTo]->autoplayEligible())
        switchTo = (switchTo + 1) % sceneCount;
    }

    if (switchTo >= 0) {
      scenes[current]->stop(ctx);
      current = switchTo;
      scenes[current]->start(ctx);
      nextFrameMs = 0;
      lastSwitchMs = ctx.nowMs;
    }

    // honor each scene's requested frame delay, like the device loop will
    if (ctx.nowMs >= nextFrameMs) {
      Scene* drawScene =
          inSettings ? static_cast<Scene*>(&settingsScene) : scenes[current];
      uint32_t requestedDelay = drawScene->draw(ctx);
      // the clock overlay rides on top of whatever just drew (not settings)
      if (!inSettings) overlay::draw(ctx, settings);
      nextFrameMs = ctx.nowMs + requestedDelay;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        RGB c = fb.at(x, y);
        c.dim(settings.brightness);
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
    DrawText(inSettings ? settingsScene.name() : scenes[current]->name(), 8,
             HEIGHT * scale - 42, 20, RAYWHITE);
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
