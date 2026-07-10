// Coriolis desktop simulator — the first display backend. Renders the
// framebuffer in a window with chunky pixels, feeds scenes the system clock,
// and maps the keyboard to the input keys a remote/gamepad will provide.
//
// Build: cmake -B build && cmake --build build   (see README.md)
#include "raylib.h"

#include "../core/config.h"
#include "../core/display.h"
#include "../core/scene.h"
#include "../scenes/scene_clock.h"
#include "../scenes/scene_plasma.h"
#include "../scenes/scene_fire.h"
#include "../scenes/scene_spiro.h"

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

int main() {
  const int scale = 8;  // chunky pixels, the whole point
  InitWindow(WIDTH * scale, HEIGHT * scale, "Coriolis");
  SetTargetFPS(120);

  FrameBuffer fb;
  SystemTime timeSource;
  int paletteIndex = 0;

  ClockScene clock;
  PlasmaScene plasma;
  FireScene fire;
  SpiroScene spiro;
  Scene* scenes[] = {&clock, &spiro, &fire, &plasma};
  const int sceneCount = sizeof(scenes) / sizeof(scenes[0]);
  int current = 0;

  Context ctx = {fb, timeSource, &palettes::byIndex(paletteIndex), 0};

  scenes[current]->start(ctx);

  uint32_t nextFrameMs = 0;
  while (!WindowShouldClose()) {
    ctx.nowMs = uint32_t(GetTime() * 1000.0);
    ctx.palette = &palettes::byIndex(paletteIndex);

    // keyboard stands in for the remote/gamepad; scenes get first refusal
    Key pressed = Key::None;
    if (IsKeyPressed(KEY_UP)) pressed = Key::Up;
    if (IsKeyPressed(KEY_DOWN)) pressed = Key::Down;
    if (IsKeyPressed(KEY_LEFT)) pressed = Key::Left;
    if (IsKeyPressed(KEY_RIGHT)) pressed = Key::Right;
    if (IsKeyPressed(KEY_ENTER)) pressed = Key::Select;
    if (IsKeyPressed(KEY_BACKSPACE)) pressed = Key::Back;

    bool consumed = false;
    if (pressed != Key::None) consumed = scenes[current]->input(ctx, pressed);

    if (!consumed) {
      if (pressed == Key::Right || IsKeyPressed(KEY_SPACE)) {
        scenes[current]->stop(ctx);
        current = (current + 1) % sceneCount;
        scenes[current]->start(ctx);
        nextFrameMs = 0;
      }
      else if (pressed == Key::Left) {
        scenes[current]->stop(ctx);
        current = (current + sceneCount - 1) % sceneCount;
        scenes[current]->start(ctx);
        nextFrameMs = 0;
      }
      else if (pressed == Key::Up) {
        paletteIndex = (paletteIndex + 1) % palettes::COUNT;
      }
      else if (pressed == Key::Down) {
        paletteIndex = (paletteIndex + palettes::COUNT - 1) % palettes::COUNT;
      }
    }

    // honor each scene's requested frame delay, like the device loop will
    if (ctx.nowMs >= nextFrameMs) {
      uint32_t requestedDelay = scenes[current]->draw(ctx);
      nextFrameMs = ctx.nowMs + requestedDelay;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        const RGB& c = fb.at(x, y);
        DrawRectangle(x * scale, y * scale, scale - 1, scale - 1,
                      Color{c.r, c.g, c.b, 255});
      }
    }
    DrawText(scenes[current]->name(), 8, 8, 20, RAYWHITE);
    DrawText(ctx.palette->name, 8, 30, 14, GRAY);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
