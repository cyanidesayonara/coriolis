// Coriolis firmware — Teensy 4.1 + SmartLED Shield V5 + 128x128 HUB75.
// The same core/ and scenes/ that run in the desktop simulator, with device
// backends for the interfaces: RTC time, EEPROM settings, serial input for
// bench bring-up (IR remote later), built-in holiday events (SD later),
// silent audio (DFPlayer later).
//
// Build: arduino-cli compile --fqbn teensy:avr:teensy41 firmware/coriolis
#include <MatrixHardware_Teensy4_ShieldV5.h>
#include <SmartMatrix.h>
#include <TimeLib.h>
#include <EEPROM.h>

// ---- SmartMatrix allocation: four 64x64 panels as a 128x128 square --------
#define COLOR_DEPTH 24
const uint16_t kMatrixWidth = 128;
const uint16_t kMatrixHeight = 128;
const uint8_t kRefreshDepth = 36;
const uint8_t kDmaBufferRows = 4;
const uint8_t kPanelType = SM_PANELTYPE_HUB75_64ROW_MOD32SCAN;
const uint32_t kMatrixOptions = SM_HUB75_OPTIONS_NONE;
const uint8_t kBackgroundLayerOptions = SM_BACKGROUND_OPTIONS_NONE;

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight,
                             kRefreshDepth, kDmaBufferRows, kPanelType,
                             kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth,
                                      kMatrixHeight, COLOR_DEPTH,
                                      kBackgroundLayerOptions);

// ---- the shared Coriolis code, unchanged from the simulator ----------------
#include <CoriolisShared.h>

using namespace coriolis;

// ---- device backends --------------------------------------------------------

// the Teensy 4.1's built-in RTC (add a coin cell to keep it through power-off)
class RtcTime : public TimeSource {
 public:
  TimeOfDay now() {
    time_t t = ::now();  // TimeLib, synced from Teensy3Clock in setup()
    TimeOfDay tod;
    tod.hour = ::hour(t);
    tod.minute = ::minute(t);
    tod.second = ::second(t);
    tod.year = ::year(t);
    tod.month = ::month(t);
    tod.day = ::day(t);
    return tod;
  }
  bool available() { return timeStatus() != timeNotSet; }
};

// bench input over USB serial until the IR receiver is wired:
//   u/d/l/r = arrows, k = OK, b = back, s = settings, c = clock overlay,
//   n = next scene. A key stays "held" briefly after each press so games
//   are steerable.
class SerialInput : public InputState {
 public:
  Key pendingEvent = Key::None;
  bool settingsKey = false, overlayKey = false, nextKey = false;

  void poll(uint32_t nowMs) {
    while (Serial.available() > 0) {
      int c = Serial.read();
      Key k = Key::None;
      switch (c) {
        case 'u': k = Key::Up; break;
        case 'd': k = Key::Down; break;
        case 'l': k = Key::Left; break;
        case 'r': k = Key::Right; break;
        case 'k': k = Key::Select; break;
        case 'b': k = Key::Back; break;
        case 's': settingsKey = true; break;
        case 'c': overlayKey = true; break;
        case 'n': nextKey = true; break;
        default: break;
      }
      if (k != Key::None) {
        pendingEvent = k;
        heldKey_ = k;
        heldUntilMs_ = nowMs + 220;
      }
    }
    if (nowMs >= heldUntilMs_) heldKey_ = Key::None;
  }

  bool isDown(Key k) const { return k == heldKey_; }

 private:
  Key heldKey_ = Key::None;
  uint32_t heldUntilMs_ = 0;
};

// settings persisted in the Teensy 4.1's emulated EEPROM
class EepromStore : public SettingsStore {
 public:
  bool load(Settings& out) {
    uint32_t magic = 0;
    EEPROM.get(0, magic);
    if (magic != MAGIC) return false;
    EEPROM.get(sizeof(magic), out);
    return true;
  }
  void save(const Settings& s) {
    uint32_t magic = MAGIC;
    EEPROM.put(0, magic);
    EEPROM.put(sizeof(magic), s);
  }

 private:
  static const uint32_t MAGIC = 0xC0410115;  // "Coriolis", settings layout v1
};

// no SD-card GIFs yet
class NullGifSource : public GifSource {
 public:
  int count() { return 0; }
  bool open(int) { return false; }
  uint32_t drawNextFrame(FrameBuffer&) { return 0; }
};

// the built-in Barcelona/Badalona holidays; personal events move to SD later
class BuiltinEvents : public EventSource {
 public:
  int count() { return BCN_HOLIDAY_COUNT; }
  const Event& get(int i) {
    return BCN_HOLIDAYS[(i < 0 || i >= BCN_HOLIDAY_COUNT) ? 0 : i];
  }
};

// ---- wiring -----------------------------------------------------------------
FrameBuffer fb;
RtcTime timeSource;
SerialInput input;
SilentAudio audio;  // DFPlayer backend comes with the hardware
Settings settings;
EepromStore store;
NullGifSource gifSource;
BuiltinEvents eventSource;
NoWeather weatherProvider;  // a UART weather feeder can replace this later

ClockScene clockScene;
AnalogClockScene analogClock;
WordClockScene wordClock;
CalendarScene calendarScene(eventSource);
WeatherScene weather(weatherProvider);
PongScene pong(settings);
SnakeScene snake(settings);
TetrisScene tetris(settings);
YogaScene yoga(settings);
ExerciseScene exercise(settings);
BreatheScene breathe(settings);
FocusScene focus;
GifScene gifs(gifSource);
SpiroScene spiro;
MandalaScene mandala;
CoriolisScene coriolisScene;
RainScene rain;
BounceScene bounce;
StarfieldScene starfield;
LifeScene life;
AquariumScene aquarium;
FireScene fire(settings);
AuroraScene aurora;
SettingsScene settingsScene(settings, store);

Scene* scenes[] = {&clockScene, &analogClock, &wordClock,    &weather,
                   &calendarScene, &pong,     &snake,        &tetris,
                   &yoga,       &exercise,    &breathe,      &focus,
                   &gifs,       &spiro,       &mandala,      &coriolisScene,
                   &rain,       &bounce,      &starfield,    &life,
                   &aquarium,   &fire,        &aurora};
const int sceneCount = sizeof(scenes) / sizeof(scenes[0]);

Context ctx = {fb, timeSource, input, audio, &palettes::byIndex(0), 0};
Toast toast;

int current = 0;
bool inSettings = false;
uint32_t nextFrameMs = 0;
uint32_t lastSwitchMs = 0;

time_t getRtcTime() { return Teensy3Clock.get(); }

void setup() {
  Serial.begin(115200);
  setSyncProvider(getRtcTime);  // TimeLib follows the hardware RTC

  store.load(settings);

  matrix.addLayer(&backgroundLayer);
  matrix.begin();
  matrix.setBrightness(settings.brightness);
  matrix.setRefreshRate(120);

  scenes[current]->start(ctx);
}

void switchScene(int to) {
  scenes[current]->stop(ctx);
  current = to;
  scenes[current]->start(ctx);
  nextFrameMs = 0;
  lastSwitchMs = ctx.nowMs;
  toast.show(scenes[current]->name(), ctx.nowMs);
}

void blit() {
  // copy the shared framebuffer to the panel, applying rotation; brightness
  // is handled globally by the matrix driver
  rgb24* out = backgroundLayer.backBuffer();
  const int W = fb.width(), H = fb.height();
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      const RGB& c = fb.at(x, y);
      int dx = x, dy = y;
      switch (settings.rotation) {
        case 1: dx = H - 1 - y; dy = x; break;
        case 2: dx = W - 1 - x; dy = H - 1 - y; break;
        case 3: dx = y; dy = W - 1 - x; break;
      }
      rgb24 px;
      px.red = c.r;
      px.green = c.g;
      px.blue = c.b;
      out[dy * kMatrixWidth + dx] = px;
    }
  }
  backgroundLayer.swapBuffers(false);
}

void loop() {
  ctx.nowMs = millis();
  ctx.palette = &palettes::byIndex(settings.paletteIndex);
  input.poll(ctx.nowMs);

  Key pressed = input.pendingEvent;
  input.pendingEvent = Key::None;

  Scene* active =
      inSettings ? static_cast<Scene*>(&settingsScene) : scenes[current];
  bool consumed = false;
  if (pressed != Key::None) consumed = active->input(ctx, pressed);

  if (input.settingsKey) {
    input.settingsKey = false;
    if (inSettings) {
      settingsScene.stop(ctx);
      inSettings = false;
    } else {
      settingsScene.openSection(scenes[current]->name());
      inSettings = true;
    }
    nextFrameMs = 0;
    consumed = true;
  }
  if (input.overlayKey) {
    input.overlayKey = false;
    settings.overlayType = uint8_t((settings.overlayType + 1) % 4);
  }

  int switchTo = -1;
  if (!consumed && inSettings) {
    if (pressed == Key::Back) {
      settingsScene.stop(ctx);
      inSettings = false;
      nextFrameMs = 0;
    }
  } else if (!consumed) {
    if (pressed == Key::Right || input.nextKey)
      switchTo = (current + 1) % sceneCount;
    else if (pressed == Key::Left)
      switchTo = (current + sceneCount - 1) % sceneCount;
    else if (pressed == Key::Up)
      settings.paletteIndex = (settings.paletteIndex + 1) % palettes::COUNT;
    else if (pressed == Key::Down)
      settings.paletteIndex =
          (settings.paletteIndex + palettes::COUNT - 1) % palettes::COUNT;
    else if (pressed == Key::Back)
      switchTo = 0;
  }
  input.nextKey = false;

  if (!inSettings && settings.autoplay && switchTo < 0 &&
      scenes[current]->autoplayEligible() &&
      ctx.nowMs - lastSwitchMs >= uint32_t(settings.autoplaySeconds) * 1000) {
    switchTo = (current + 1) % sceneCount;
    while (!scenes[switchTo]->autoplayEligible())
      switchTo = (switchTo + 1) % sceneCount;
  }

  if (switchTo >= 0) switchScene(switchTo);

  if (ctx.nowMs >= nextFrameMs) {
    Scene* drawScene =
        inSettings ? static_cast<Scene*>(&settingsScene) : scenes[current];
    uint32_t requestedDelay = drawScene->draw(ctx);
    if (!inSettings) overlay::draw(ctx, settings);
    toast.draw(ctx);
    matrix.setBrightness(settings.brightness);
    blit();
    nextFrameMs = ctx.nowMs + requestedDelay;
  }
}
