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
#include "../scenes/scene_yoga.h"
#include "../scenes/scene_exercise.h"
#include "../scenes/scene_breathe.h"
#include "../scenes/scene_gifs.h"
#include "../scenes/scene_plasma.h"
#include "../scenes/scene_fire.h"
#include "../scenes/scene_spiro.h"
#include "../scenes/scene_rain.h"
#include "../scenes/scene_settings.h"

#include <cstdio>
#include <cstring>
#include <ctime>

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

int main() {
  // chunky pixels, the whole point — but keep the window on a 1080p screen
  int scale = 8;
  while (scale > 2 && (WIDTH * scale > 1700 || HEIGHT * scale > 900)) scale--;
  InitWindow(WIDTH * scale, HEIGHT * scale, "Coriolis");
  SetTargetFPS(120);

  FrameBuffer fb;
  SystemTime timeSource;
  KeyboardInput heldKeys;

  Settings settings;
  FileSettingsStore store;
  store.load(settings);

  RaylibGifSource gifSource;

  ClockScene clock;
  AnalogClockScene analogClock;
  WordClockScene wordClock;
  PongScene pong(settings);
  YogaScene yoga(settings);
  ExerciseScene exercise(settings);
  BreatheScene breathe(settings);
  GifScene gifs(gifSource);
  SpiroScene spiro;
  RainScene rain;
  FireScene fire(settings);
  PlasmaScene plasma;
  SettingsScene settingsScene(settings, store);

  // settings is deliberately NOT in the rotation: it opens on its own key,
  // so it can't be stumbled into or autoplayed through
  Scene* scenes[] = {&clock,    &analogClock, &wordClock, &pong,
                     &yoga,     &exercise,    &breathe,   &gifs,
                     &spiro,    &rain,        &fire,      &plasma};
  const int sceneCount = sizeof(scenes) / sizeof(scenes[0]);
  int current = 0;
  bool inSettings = false;

  Context ctx = {fb, timeSource, heldKeys,
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
    EndDrawing();
  }

  store.save(settings);
  CloseWindow();
  return 0;
}
