// Headless checks for the shared code: color math, calendar/time-zone
// rules, the shell's navigation (launcher/settings/scene key routing) and
// the guided sessions' audio contract. No window, no raylib — just run it:
//   cmake --build build --config Release && build\Release\coriolis_tests.exe
#include <cstdio>
#include <cstring>
#include <math.h>

#include "../core/color.h"
#include "../core/timeutil.h"
#include "../core/settings.h"
#include "../scenes/shell.h"
#include "../scenes/scene_clock.h"
#include "../scenes/scene_yoga.h"
#include "../scenes/scene_exercise.h"
#include "../scenes/scene_labyrinth.h"
#include "../scenes/scene_pong.h"
#include "../scenes/scene_snake.h"
#include "../scenes/scene_tetris.h"
#include "../scenes/scene_breathe.h"
#include "../scenes/scene_focus.h"

using namespace coriolis;

static int failures = 0;
#define CHECK(cond)                                                   \
  do {                                                                \
    if (!(cond)) {                                                    \
      printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);          \
      failures++;                                                     \
    }                                                                 \
  } while (0)

class FixedTime : public TimeSource {
 public:
  int hour = 9;
  TimeOfDay now() { return {hour, 30, 0, 2026, 10, 26}; }
};

class NoInput : public InputState {
 public:
  bool isDown(Key) const { return false; }
};

// records every cue and voice clip a scene fires
class RecordingAudio : public AudioSink {
 public:
  int voices[512];
  int nVoices = 0;
  int cues[512];
  int nCues = 0;
  void play(Cue c) { if (nCues < 512) cues[nCues++] = int(c); }
  void voice(int n) { if (nVoices < 512) voices[nVoices++] = n; }
  bool spoke(int n) const {
    for (int i = 0; i < nVoices; i++) if (voices[i] == n) return true;
    return false;
  }
  bool fired(Cue c) const {
    for (int i = 0; i < nCues; i++) if (cues[i] == int(c)) return true;
    return false;
  }
};

class NullStore : public SettingsStore {
 public:
  bool load(Settings&) { return false; }
  void save(const Settings&) {}
};

class CountingStore : public SettingsStore {
 public:
  int saves = 0;
  Settings saved;
  bool load(Settings&) { return false; }
  void save(const Settings& s) { saves++; saved = s; }
};

// a scene that wants every arrow key, like a game in progress
class GreedyScene : public Scene {
 public:
  int keys = 0;
  int starts = 0;
  const char* name() const { return "Tetris"; }
  void start(Context&) { starts++; }
  bool input(Context&, Key) { keys++; return true; }
  uint32_t draw(Context& ctx) { ctx.fb.clear(); return 33; }
};

class PlainScene : public Scene {
 public:
  explicit PlainScene(const char* n) : n_(n) {}
  const char* name() const { return n_; }
  uint32_t draw(Context& ctx) { ctx.fb.clear(); return 33; }
 private:
  const char* n_;
};

static void testColor() {
  CHECK(lerp8(255, 195, 128) == 225);  // the old version wrapped to 97
  CHECK(lerp8(0, 200, 128) == 100);
  CHECK(lerp8(200, 0, 255) < 2);
  CHECK(lerp8(10, 10, 77) == 10);
  // gradient palettes hold their top color instead of wrapping to black
  RGB top = palettes::byIndex(1).lookup(255);  // Heat
  CHECK(top.r > 200 && top.g > 200);
}

static void testTime() {
  using namespace timeutil;
  CHECK(daysFromCivil(1970, 1, 1) == 0);
  CHECK(weekdayOf(2026, 10, 26) == 1);  // a Monday
  int y, m, d;
  civilFromDays(daysFromCivil(2028, 2, 29), y, m, d);
  CHECK(y == 2028 && m == 2 && d == 29);
  // EU summer time 2026: 29 March 01:00 UTC .. 25 October 01:00 UTC
  CHECK(!euSummerTime(toEpoch(2026, 3, 29, 0, 59, 59)));
  CHECK(euSummerTime(toEpoch(2026, 3, 29, 1, 0, 0)));
  CHECK(euSummerTime(toEpoch(2026, 10, 25, 0, 59, 59)));
  CHECK(!euSummerTime(toEpoch(2026, 10, 25, 1, 0, 0)));
  CHECK(localOffset(toEpoch(2026, 7, 1, 12, 0, 0), 1, true) == 7200);
  CHECK(localOffset(toEpoch(2026, 12, 1, 12, 0, 0), 1, true) == 3600);
  CHECK(localOffset(toEpoch(2026, 7, 1, 12, 0, 0), 1, false) == 3600);
  int h, mi, s;
  fromEpoch(toEpoch(2026, 10, 26, 13, 45, 7), y, m, d, h, mi, s);
  CHECK(y == 2026 && m == 10 && d == 26 && h == 13 && mi == 45 && s == 7);
}

static void testNight() {
  Settings s;
  s.brightness = 200;
  CHECK(s.isNightTime(23) && s.isNightTime(3) && !s.isNightTime(7));
  CHECK(s.effectiveBrightness(2) == s.nightBrightness);
  CHECK(s.effectiveBrightness(12) == 200);

  // menus enforce a >= 96 brightness floor at night so Settings/Launcher stay readable
  FrameBuffer fb;
  FixedTime time;
  time.hour = 23;
  NoInput held;
  RecordingAudio audio;
  NullStore store;
  SettingsScene settingsScene(s, store);
  PlainScene clock("Clock");
  Scene* list[] = {&clock};
  Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 0};
  Shell shell(list, 1, s, settingsScene);
  shell.begin(ctx, false);
  CHECK(shell.isNightActive(ctx));
  CHECK(shell.brightness(ctx) == s.nightBrightness);
  shell.command(ctx, Command::Settings);
  CHECK(shell.brightness(ctx) >= 96);
  shell.key(ctx, Key::Back);
  CHECK(shell.brightness(ctx) == s.nightBrightness);
  shell.command(ctx, Command::Menu);
  CHECK(shell.brightness(ctx) >= 96);
}

static void testSanitize() {
  Settings s;
  s.exerciseRepSec = 0;   // a divisor
  s.yogaHoldSec = 0;      // a divisor
  s.overlayType = 9;      // a table index
  s.overlayPos = 200;
  s.volume = 99;
  s.sanitize();
  CHECK(s.exerciseRepSec == 3 && s.yogaHoldSec == 15);
  CHECK(s.overlayType == 0 && s.overlayPos == 4 && s.volume == 20);
}

static void testShell() {
  FrameBuffer fb;
  FixedTime time;
  NoInput held;
  RecordingAudio audio;
  Settings settings;
  NullStore store;
  SettingsScene settingsScene(settings, store);
  PlainScene clock("Clock"), aurora("Aurora");
  GreedyScene game;
  Scene* list[] = {&clock, &game, &aurora};
  Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 0};
  Shell shell(list, 3, settings, settingsScene);
  shell.begin(ctx, true);

  // any key skips the splash and lands on the clock
  shell.key(ctx, Key::Select);
  shell.tick(ctx);
  CHECK(shell.current() == 0);
  CHECK(strcmp(shell.activeName(), "Clock") == 0);

  // right goes to the next scene
  shell.key(ctx, Key::Right);
  CHECK(shell.current() == 1);

  // the game swallows arrows... until the launcher opens: then the launcher
  // must get them, not the game underneath (a bug in the old main loops)
  shell.key(ctx, Key::Up);
  CHECK(game.keys == 1);
  shell.command(ctx, Command::Menu);
  CHECK(strcmp(shell.activeName(), "Launcher") == 0);
  shell.key(ctx, Key::Down);
  shell.key(ctx, Key::Up);
  CHECK(game.keys == 1);

  // Games tab -> the game is preselected; Art tab -> Aurora; OK launches
  shell.key(ctx, Key::Right);
  shell.key(ctx, Key::Select);
  CHECK(shell.current() == 2);
  CHECK(!shell.menuOpen());

  // Menu again closes the launcher without switching
  shell.command(ctx, Command::Menu);
  shell.command(ctx, Command::Menu);
  CHECK(!shell.menuOpen() && shell.current() == 2);

  // settings open and close; Back closes them too
  shell.command(ctx, Command::Settings);
  CHECK(strcmp(shell.activeName(), "Settings") == 0);
  shell.key(ctx, Key::Back);
  CHECK(!shell.menuOpen());

  // picking the scene that's already showing must not restart it (it would
  // throw away a game or workout in progress)
  {
    int before = game.starts;
    shell.show(ctx, 1);
    before = game.starts;
    shell.command(ctx, Command::Menu);
    shell.key(ctx, Key::Select);
    CHECK(game.starts == before);
    CHECK(!shell.menuOpen() && shell.current() == 1);
  }

  // Home from anywhere
  shell.command(ctx, Command::Home);
  CHECK(shell.current() == 0);
  CHECK(!shell.key(ctx, Key::Back));  // Back at Clock with no menu open is a no-op
  shell.show(ctx, 2);                 // PlainScene (Aurora)
  CHECK(shell.current() == 2);
  CHECK(shell.key(ctx, Key::Back));   // backing out from non-0 scene returns true
  CHECK(shell.current() == 0);

  // frames keep coming through a transition
  int frames = 0;
  for (uint32_t t = 0; t < 1000; t += 8) {
    ctx.nowMs = t + 100;
    if (shell.tick(ctx)) frames++;
  }
  CHECK(frames > 20);
}

// run a guided scene through its whole session by skipping ahead
template <class S>
static void runSession(S& scene, Context& ctx, int skips) {
  scene.start(ctx);
  scene.draw(ctx);
  scene.input(ctx, Key::Select);  // setup card -> countdown
  for (uint32_t t = 0; t < 3200; t += 33) {
    ctx.nowMs += 33;
    scene.draw(ctx);
  }
  for (int i = 0; i < skips; i++) {
    ctx.nowMs += 500;
    scene.input(ctx, Key::Right);
    scene.draw(ctx);
  }
  ctx.nowMs += 100;
  scene.draw(ctx);
}

static bool voicesInRange(const RecordingAudio& a) {
  for (int i = 0; i < a.nVoices; i++)
    if (a.voices[i] < 1 || a.voices[i] > 255) return false;
  return true;
}

static void testGuides() {
  FrameBuffer fb;
  FixedTime time;
  NoInput held;
  for (int p = 0; p < YogaScene::FLOW_COUNT; p++) {
    RecordingAudio audio;
    Settings s;
    s.yogaProgram = uint8_t(p);
    Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 1000};
    YogaScene yoga(s);
    runSession(yoga, ctx, 20);
    CHECK(audio.spoke(voice::NAMASTE));
    CHECK(audio.fired(Cue::FinishBell));
    CHECK(voicesInRange(audio));
    if (p == 2) CHECK(audio.spoke(voice::SWITCH_SIDES));  // standing flow
  }
  for (int p = 0; p < ExerciseScene::PROGRAM_COUNT; p++) {
    RecordingAudio audio;
    Settings s;
    s.exerciseProgram = uint8_t(p);
    Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 1000};
    ExerciseScene ex(s);
    runSession(ex, ctx, 40);
    CHECK(audio.spoke(voice::COMPLETE));
    CHECK(voicesInRange(audio));
  }
  {  // the kettlebell program says "kettlebell swings" (106), not "squats"
    RecordingAudio audio;
    Settings s;
    s.exerciseProgram = 1;
    Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 1000};
    ExerciseScene ex(s);
    runSession(ex, ctx, 0);
    CHECK(audio.spoke(voice::MOVE_BASE + 6));
    CHECK(!audio.spoke(voice::MOVE_BASE + 1));
  }
}

static void testEscapes() {
  FrameBuffer fb;
  FixedTime time;
  NoInput held;
  RecordingAudio audio;
  Settings s;
  Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 1000};

  // Labyrinth: Back escapes to setup card; Select pause menu -> Exit also escapes
  {
    LabyrinthScene lab;
    lab.start(ctx);
    CHECK(!lab.input(ctx, Key::Right));  // setup card lets Right switch scene
    CHECK(lab.input(ctx, Key::Select));  // enter maze
    CHECK(lab.input(ctx, Key::Right));   // Right consumed while exploring
    CHECK(lab.input(ctx, Key::Back));    // Back exits to setup card
    CHECK(!lab.input(ctx, Key::Right));  // Right now switches scene again

    CHECK(lab.input(ctx, Key::Select));  // re-enter maze
    CHECK(lab.input(ctx, Key::Select));  // open pause menu (row 0 = Resume)
    CHECK(lab.input(ctx, Key::Down));    // row 1 = New maze
    CHECK(lab.input(ctx, Key::Down));    // row 2 = Exit
    CHECK(lab.input(ctx, Key::Select));  // confirm Exit
    CHECK(!lab.input(ctx, Key::Left));   // un-stuck!
  }

  // Games, Breathe, Focus: Back exits active mode back to setup card
  {
    PongScene pong(s);
    pong.start(ctx);
    CHECK(pong.input(ctx, Key::Select));
    CHECK(pong.input(ctx, Key::Left));
    CHECK(pong.input(ctx, Key::Back));
    CHECK(!pong.input(ctx, Key::Left));
  }
  {
    SnakeScene snake(s);
    snake.start(ctx);
    CHECK(snake.input(ctx, Key::Select));
    CHECK(snake.input(ctx, Key::Right));
    CHECK(snake.input(ctx, Key::Back));
    CHECK(!snake.input(ctx, Key::Right));
  }
  {
    TetrisScene tetris(s);
    tetris.start(ctx);
    CHECK(tetris.input(ctx, Key::Select));
    CHECK(tetris.input(ctx, Key::Left));
    CHECK(tetris.input(ctx, Key::Back));
    CHECK(!tetris.input(ctx, Key::Left));
  }
  {
    BreatheScene breathe(s);
    breathe.start(ctx);
    CHECK(breathe.input(ctx, Key::Select));
    CHECK(breathe.input(ctx, Key::Back));
    CHECK(!breathe.input(ctx, Key::Right));
  }
  {
    FocusScene focus;
    focus.start(ctx);
    CHECK(focus.input(ctx, Key::Select));
    CHECK(focus.input(ctx, Key::Back));
    CHECK(!focus.input(ctx, Key::Right));
  }
}

static float dist2D(const float a[2], const float b[2]) {
  return hypotf(a[0] - b[0], a[1] - b[1]);
}

static void testKinematics() {
  // verify arc-preserving arm lengths and planted ankles across mid-rep t
  const Exercise* cases[2] = {&exercises::KB_SWING, &exercises::JACKS};
  const float ts[3] = {0.25f, 0.5f, 0.75f};
  for (int c = 0; c < 2; c++) {
    const Exercise& ex = *cases[c];
    float uaA = dist2D(ex.a[2], ex.a[4]), uaB = dist2D(ex.b[2], ex.b[4]);
    float faA = dist2D(ex.a[4], ex.a[5]), faB = dist2D(ex.b[4], ex.b[5]);
    float uaR = dist2D(ex.a[3], ex.a[6]), uaRB = dist2D(ex.b[3], ex.b[6]);
    float faR = dist2D(ex.a[6], ex.a[7]), faRB = dist2D(ex.b[6], ex.b[7]);
    for (int k = 0; k < 3; k++) {
      float t = ts[k];
      float j[15][2];
      guide::tweenPoseKinematic(ex.a, ex.b, t, j);
      CHECK(fabsf(dist2D(j[2], j[4]) - (uaA + (uaB - uaA) * t)) < 1e-4f);
      CHECK(fabsf(dist2D(j[4], j[5]) - (faA + (faB - faA) * t)) < 1e-4f);
      CHECK(fabsf(dist2D(j[3], j[6]) - (uaR + (uaRB - uaR) * t)) < 1e-4f);
      CHECK(fabsf(dist2D(j[6], j[7]) - (faR + (faRB - faR) * t)) < 1e-4f);
    }
  }
  // planted ankles in SQUAT stay fixed on the mat at y = 0.90
  for (int k = 0; k < 3; k++) {
    float j[15][2];
    guide::tweenPoseKinematic(exercises::SQUAT.a, exercises::SQUAT.b, ts[k], j);
    CHECK(fabsf(j[10][1] - 0.90f) < 1e-4f);
    CHECK(fabsf(j[12][1] - 0.90f) < 1e-4f);
  }
}

static void testPowerAndAutosave() {
  FrameBuffer fb;
  fb.clear();
  CHECK(powerLimitedBrightness(fb, 255, 25000, 32000) == 255);

  fb.fill(RGB(255, 255, 255));
  CHECK(powerLimitedBrightness(fb, 200, 0, 32000) == 200);
  uint8_t capped = powerLimitedBrightness(fb, 255, 16000, 32000);
  CHECK(capped >= 126 && capped <= 128);

  FixedTime time;
  NoInput held;
  RecordingAudio audio;
  Settings settings;
  CountingStore store;
  SettingsScene settingsScene(settings, store);
  PlainScene clock("Clock");
  Scene* list[] = {&clock};
  Context ctx = {fb, time, held, audio, &palettes::byIndex(0), 100};
  Shell shell(list, 1, settings, settingsScene);
  shell.begin(ctx, false);
  shell.tick(ctx);
  CHECK(store.saves == 0);

  settings.snakeHigh = 12;
  ctx.nowMs = 1000;
  shell.tick(ctx);
  CHECK(store.saves == 0);

  settings.tetrisHigh = 500;
  ctx.nowMs = 2500;
  shell.tick(ctx);
  CHECK(store.saves == 0);

  ctx.nowMs = 3100;
  shell.tick(ctx);
  CHECK(store.saves == 1);
  CHECK(store.saved.snakeHigh == 12 && store.saved.tetrisHigh == 500);

  ctx.nowMs = 6000;
  shell.tick(ctx);
  CHECK(store.saves == 1);
}

int main() {
  testColor();
  testTime();
  testNight();
  testSanitize();
  testShell();
  testGuides();
  testEscapes();
  testKinematics();
  testPowerAndAutosave();
  if (failures) {
    printf("%d check(s) failed\n", failures);
    return 1;
  }
  printf("all checks passed\n");
  return 0;
}
