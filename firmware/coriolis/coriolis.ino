// Coriolis firmware — Teensy 4.1 + SmartLED Shield V5 + 128x128 HUB75.
// The same core/ and scenes/ that run in the desktop simulator — including
// the Shell that does scene switching, menus and transitions — with device
// backends for the interfaces: the RTC (kept in UTC, shown in local time
// with automatic EU summer time), EEPROM settings, an IR remote plus USB
// serial for input, a DFPlayer Mini for sound, and built-in holidays.
//
// Pins: the SmartLED Shield V5 owns 2 (OE), 3 (LAT), 6, 7 (CLK), 9-13 and
// drives the panels' address lines through its own latch. Coriolis adds:
//   IR receiver OUT  -> pin 16
//   DFPlayer TX/RX   -> pins 0 (RX1) / 1 (TX1, through 1k)   [Serial1]
// (pin 13 is panel data, so the Teensy's LED flickers — that's normal)
//
// Build: arduino-cli compile --fqbn teensy:avr:teensy41 firmware/coriolis
#include <MatrixHardware_Teensy4_ShieldV5.h>
#include <SmartMatrix.h>
#include <TimeLib.h>
#include <EEPROM.h>

// ---- SmartMatrix allocation: four 64x64 panels as a 128x128 square --------
// Panel chaining (see docs/BUILD.md, "Panel layout"): with no options all
// four panels hang upright and the chain runs across the top row, then the
// bottom row. SM_HUB75_OPTIONS_C_SHAPE_STACKING snakes the chain instead
// (shorter cables; the second row is mounted upside down), and
// SM_HUB75_OPTIONS_BOTTOM_TO_TOP_STACKING starts from the bottom row. Use
// the serial 't' test pattern to check the result.
#ifndef CORIOLIS_PANEL_OPTIONS
#define CORIOLIS_PANEL_OPTIONS SM_HUB75_OPTIONS_NONE
#endif
// panels with FM6126A drivers stay dark without a reset at start:
//   #define CORIOLIS_PANEL_OPTIONS SM_HUB75_OPTIONS_FM6126A_RESET_AT_START

// a hard ceiling on panel brightness, for a smaller power supply (255 = no
// cap; ~150 keeps four panels near 12A even on full white)
#ifndef CORIOLIS_MAX_BRIGHTNESS
#define CORIOLIS_MAX_BRIGHTNESS 255
#endif

#define COLOR_DEPTH 24
const uint16_t kMatrixWidth = 128;
const uint16_t kMatrixHeight = 128;
const uint8_t kRefreshDepth = 36;
const uint8_t kDmaBufferRows = 4;
const uint8_t kPanelType = SM_PANELTYPE_HUB75_64ROW_MOD32SCAN;
const uint32_t kMatrixOptions = CORIOLIS_PANEL_OPTIONS;
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

const int kIrPin = 16;
#define DFPLAYER_SERIAL Serial1

// a stamp unique to this build, to notice the first boot after flashing
static uint32_t buildStamp() {
  const char* s = __DATE__ " " __TIME__;
  uint32_t h = 2166136261u;
  while (*s) { h ^= uint8_t(*s++); h *= 16777619u; }
  return h;
}

// ---- time -------------------------------------------------------------------

// The RTC keeps UTC; the display shows local time from the zone and EU
// summer-time settings, so daylight saving needs no attention. (Add a CR2032
// on VBAT to keep time through power cuts.)
class RtcTime : public TimeSource {
 public:
  explicit RtcTime(const Settings& s) : s_(s) {}

  TimeOfDay now() {
    int64_t utc = int64_t(::now());  // TimeLib, synced from Teensy3Clock
    int64_t local = utc + timeutil::localOffset(utc, s_.zoneHours, s_.euDst);
    TimeOfDay t;
    timeutil::fromEpoch(local, t.year, t.month, t.day, t.hour, t.minute,
                        t.second);
    return t;
  }
  bool available() { return timeStatus() != timeNotSet; }
  bool settable() const { return true; }

  void setLocal(const TimeOfDay& t) {
    int64_t local = timeutil::toEpoch(t.year, t.month, t.day, t.hour,
                                      t.minute, t.second);
    setUtc(local - offsetForLocal(local));
  }

  void setUtc(int64_t utc) {
    Teensy3Clock.set(time_t(utc));
    setTime(time_t(utc));
  }

  // the Teensy Loader sets the RTC to the PC's *local* time on upload;
  // reinterpret that once as local and store UTC
  void adoptLoaderTime() {
    int64_t local = int64_t(Teensy3Clock.get());
    setUtc(local - offsetForLocal(local));
  }

 private:
  const Settings& s_;
  int32_t offsetForLocal(int64_t local) const {
    // the offset depends on the UTC instant; standard time gets us there
    return timeutil::localOffset(local - s_.zoneHours * 3600, s_.zoneHours,
                                 s_.euDst);
  }
};

// ---- IR remote ----------------------------------------------------------------

// Interrupt-driven NEC decoder. Times the gaps between falling edges of the
// TSOP output: a 13.5ms gap starts a frame, 11.25ms is a "still held"
// repeat, then 1.125ms / 2.25ms gaps are 0 / 1 bits, LSB first.
class IrReceiver {
 public:
  static void begin(int pin) {
    pinMode(pin, INPUT);  // the TSOP has its own pull-up
    attachInterrupt(digitalPinToInterrupt(pin), isr, FALLING);
  }

  // a complete frame since the last call?
  static bool frame(uint32_t& code) {
    if (!ready_) return false;
    noInterrupts();
    code = code_;
    ready_ = false;
    interrupts();
    return true;
  }

  // millis() of the last frame or repeat — the button is held while fresh
  static uint32_t lastSeenMs() { return lastSeenMs_; }

 private:
  static volatile uint32_t code_, buf_, lastEdgeUs_, lastSeenMs_;
  static volatile int bits_;
  static volatile bool ready_;

  static void isr() {
    uint32_t now = micros();
    uint32_t d = now - lastEdgeUs_;
    lastEdgeUs_ = now;
    if (d > 12500 && d < 14500) {         // leader: a new frame
      bits_ = 0;
      buf_ = 0;
    } else if (d > 10500 && d < 12000) {  // repeat code
      bits_ = -1;
      lastSeenMs_ = millis();
    } else if (bits_ >= 0 && bits_ < 32) {
      if (d > 800 && d < 1500) {
        buf_ >>= 1;
      } else if (d > 1800 && d < 2700) {
        buf_ = (buf_ >> 1) | 0x80000000u;
      } else {
        bits_ = -1;  // noise: drop the frame
        return;
      }
      if (++bits_ == 32) {
        code_ = buf_;
        ready_ = true;
        lastSeenMs_ = millis();
        bits_ = -1;
      }
    }
  }
};

volatile uint32_t IrReceiver::code_ = 0;
volatile uint32_t IrReceiver::buf_ = 0;
volatile uint32_t IrReceiver::lastEdgeUs_ = 0;
volatile uint32_t IrReceiver::lastSeenMs_ = 0;
volatile int IrReceiver::bits_ = -1;
volatile bool IrReceiver::ready_ = false;

enum class Button : uint8_t {
  None, Up, Down, Left, Right, Ok, Menu, Play, Back, Star, Zero,
  Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9
};

// Apple remotes (aluminium A1294, white A1156) and the common 17-key NEC
// "Arduino kit" remote. Anything else is printed on USB serial so it can be
// added here. (A struct, so the Arduino prototype generator leaves it be.)
struct RemoteMap {
static Button decode(uint32_t code) {
  uint8_t b0 = code & 0xFF, b1 = (code >> 8) & 0xFF;
  uint8_t b2 = (code >> 16) & 0xFF, b3 = (code >> 24) & 0xFF;
  if (b0 == 0xEE && b1 == 0x87) {  // Apple: the low bit of the command is parity
    switch (b2 & 0xFE) {
      case 0x0A: return Button::Up;
      case 0x0C: return Button::Down;
      case 0x08: return Button::Left;
      case 0x06: return Button::Right;
      case 0x5C: return Button::Ok;     // aluminium center
      case 0x04: return Button::Ok;     // white remote's center/play
      case 0x02: return Button::Menu;
      case 0x5E: return Button::Play;   // aluminium play/pause
      default: return Button::None;
    }
  }
  if (uint8_t(b2 ^ b3) != 0xFF) return Button::None;  // NEC: cmd, ~cmd
  if (b0 != 0x00) return Button::None;
  switch (b2) {
    case 0x18: return Button::Up;
    case 0x52: return Button::Down;
    case 0x08: return Button::Left;
    case 0x5A: return Button::Right;
    case 0x1C: return Button::Ok;
    case 0x16: return Button::Star;   // '*' = menu
    case 0x0D: return Button::Back;   // '#'
    case 0x19: return Button::Zero;   // '0' = clock overlay
    case 0x45: return Button::Digit1;
    case 0x46: return Button::Digit2;
    case 0x47: return Button::Digit3;
    case 0x44: return Button::Digit4;
    case 0x40: return Button::Digit5;
    case 0x43: return Button::Digit6;
    case 0x07: return Button::Digit7;
    case 0x15: return Button::Digit8;
    case 0x09: return Button::Digit9;
    default: return Button::None;
  }
}
};

// ---- input: IR + USB serial -> shell keys and commands ----------------------
class DeviceInput : public InputState {
 public:
  bool isDown(Key k) const { return k != Key::None && k == heldKey_; }
  bool testPattern = false;

  void poll(Shell& shell, Context& ctx, RtcTime& rtc) {
    uint32_t now = ctx.nowMs;
    pollIr(shell, ctx, now);
    pollSerial(shell, ctx, rtc, now);
    if (now >= heldUntilMs_) heldKey_ = Key::None;
  }

 private:
  Key heldKey_ = Key::None;
  uint32_t heldUntilMs_ = 0;
  Button irButton_ = Button::None;  // the button currently held on the remote
  uint32_t irDownMs_ = 0;
  uint32_t nextRepeatMs_ = 0;
  bool longFired_ = false;
  char line_[24];
  int lineLen_ = 0;

  static Key keyFor(Button b) {
    switch (b) {
      case Button::Up: return Key::Up;
      case Button::Down: return Key::Down;
      case Button::Left: return Key::Left;
      case Button::Right: return Key::Right;
      case Button::Ok: return Key::Select;
      case Button::Back: return Key::Back;
      default: return Key::None;
    }
  }

  void press(Shell& shell, Context& ctx, Key k, uint32_t now) {
    shell.key(ctx, k);
    heldKey_ = k;
    heldUntilMs_ = now + 220;
  }

  void pollIr(Shell& shell, Context& ctx, uint32_t now) {
    uint32_t code;
    if (IrReceiver::frame(code)) {
      Button b = RemoteMap::decode(code);
      Serial.printf("[IR] 0x%08lX -> %d\n", (unsigned long)code, int(b));
      if (b != Button::None) {
        irButton_ = b;
        irDownMs_ = now;
        nextRepeatMs_ = now + 450;
        longFired_ = false;
        Key k = keyFor(b);
        if (k != Key::None) press(shell, ctx, k, now);
        // Menu/Star/Play act on release (short) or after a hold (long)
        switch (b) {
          case Button::Zero: shell.command(ctx, Command::Overlay); break;
          case Button::Digit1: case Button::Digit2: case Button::Digit3:
          case Button::Digit4: case Button::Digit5: case Button::Digit6:
          case Button::Digit7: case Button::Digit8: case Button::Digit9:
            jumpToDigit(shell, ctx, int(b) - int(Button::Digit1) + 1);
            break;
          default: break;
        }
      }
    }
    if (irButton_ == Button::None) return;

    bool held = now - IrReceiver::lastSeenMs() < 160;
    bool menuLike = irButton_ == Button::Menu || irButton_ == Button::Star ||
                    irButton_ == Button::Play;
    if (held) {
      Key k = keyFor(irButton_);
      if (k != Key::None) {  // keep games steering; auto-repeat for menus
        heldKey_ = k;
        heldUntilMs_ = now + 160;
        if (now >= nextRepeatMs_ && shell.menuOpen()) {
          shell.key(ctx, k);
          nextRepeatMs_ = now + 110;
        }
      }
      if (menuLike && !longFired_ && now - irDownMs_ >= 800) {
        longFired_ = true;  // hold Menu: settings; hold Play: home
        shell.command(ctx, irButton_ == Button::Play ? Command::Home
                                                     : Command::Settings);
      }
      return;
    }
    // released
    if (menuLike && !longFired_) {
      if (irButton_ == Button::Play) shell.key(ctx, Key::Select);
      else shell.command(ctx, Command::Menu);
    }
    irButton_ = Button::None;
  }

  // number keys jump straight to favourite scenes (by name, not position)
  static void jumpToDigit(Shell& shell, Context& ctx, int digit);

  // bench input over USB serial:
  //   u/d/l/r arrows, k OK, b back, m menu, s settings, c clock overlay,
  //   n next, h home, t panel test pattern, i info,
  //   T<unix seconds, UTC> sets the clock, e.g. "T1793000000"
  void pollSerial(Shell& shell, Context& ctx, RtcTime& rtc, uint32_t now) {
    while (Serial.available() > 0) {
      int c = Serial.read();
      if (lineLen_ > 0 || c == 'T') {  // collecting a time-set line
        if (c == '\n' || c == '\r') {
          line_[lineLen_] = 0;
          long long t = atoll(line_ + 1);
          if (t > 1600000000LL) {
            rtc.setUtc(t);
            Serial.println("[time] set");
            shell.toast().show("Time set", now);
          }
          lineLen_ = 0;
        } else if (lineLen_ < int(sizeof(line_)) - 1) {
          line_[lineLen_++] = char(c);
        }
        continue;
      }
      switch (c) {
        case 'u': press(shell, ctx, Key::Up, now); break;
        case 'd': press(shell, ctx, Key::Down, now); break;
        case 'l': press(shell, ctx, Key::Left, now); break;
        case 'r': press(shell, ctx, Key::Right, now); break;
        case 'k': press(shell, ctx, Key::Select, now); break;
        case 'b': press(shell, ctx, Key::Back, now); break;
        case 'm': shell.command(ctx, Command::Menu); break;
        case 's': shell.command(ctx, Command::Settings); break;
        case 'c': shell.command(ctx, Command::Overlay); break;
        case 'n': shell.command(ctx, Command::Next); break;
        case 'h': shell.command(ctx, Command::Home); break;
        case 't': testPattern = !testPattern; break;
        case 'i': {
          TimeOfDay t = ctx.time.now();
          Serial.printf("[info] %04d-%02d-%02d %02d:%02d:%02d local, scene %s, refresh lowered: %d\n",
                        t.year, t.month, t.day, t.hour, t.minute, t.second,
                        shell.activeName(), int(matrix.getRefreshRateLoweredFlag()));
          break;
        }
        default: break;
      }
    }
  }
};

// ---- audio: DFPlayer Mini on Serial1 -------------------------------------------

// Plays numbered clips from the module's own microSD (see docs/AUDIO.md):
// cues are 01/NNN, voice clips 02/NNN. The module takes one command at a
// time, so commands queue and go out spaced apart, and a voice clip waits
// for the chime fired in the same breath to ring first. While a loop (the
// fireplace crackle) plays, one-shots go out as "advertisements", which the
// DFPlayer mixes in by pausing and resuming the loop.
class DfPlayerAudio : public AudioSink {
 public:
  void begin() {
    DFPLAYER_SERIAL.begin(9600);
    readyMs_ = millis() + 1500;  // the module boots after power-up
  }

  void play(Cue c) {
    int track = trackFor(c);
    if (!track || muted_) return;
    if (looping_) enqueue(0x13, uint16_t(track));        // ADVERT/000N
    else enqueue(0x0F, uint16_t((1 << 8) | track));      // 01/00N
    lastCueMs_ = millis();
  }

  void voice(int n) {
    if (n < 1 || n > 255 || muted_) return;
    pendingVoice_ = n;
    voiceDueMs_ = millis() + (millis() - lastCueMs_ < 600 ? 650 : 0);
  }

  void loop(Cue c) {
    if (c == Cue::None) {
      if (looping_) enqueue(0x16, 0);  // stop
      looping_ = false;
      return;
    }
    int track = trackFor(c);
    if (!track || muted_) return;
    enqueue(0x0F, uint16_t((1 << 8) | track));
    enqueue(0x19, 0);  // repeat the current track
    looping_ = true;
  }

  void setVolume(uint8_t v) {
    muted_ = v == 0;
    enqueue(0x06, v > 30 ? 30 : v);
  }

  void update(uint32_t) {
    uint32_t now = millis();
    if (pendingVoice_ && now >= voiceDueMs_) {
      enqueue(0x0F, uint16_t((2 << 8) | pendingVoice_));
      pendingVoice_ = 0;
    }
    if (count_ == 0 || now < readyMs_ || now - lastSendMs_ < 60) return;
    Cmd c = q_[head_];
    head_ = (head_ + 1) % QN;
    count_--;
    send(c.cmd, c.arg);
    lastSendMs_ = now;
  }

 private:
  static const int QN = 8;
  struct Cmd { uint8_t cmd; uint16_t arg; };
  Cmd q_[QN];
  int head_ = 0, count_ = 0;
  uint32_t readyMs_ = 0, lastSendMs_ = 0, lastCueMs_ = 0, voiceDueMs_ = 0;
  int pendingVoice_ = 0;
  bool looping_ = false, muted_ = false;

  void enqueue(uint8_t cmd, uint16_t arg) {
    if (count_ == QN) {  // full: drop the oldest, the newest matters more
      head_ = (head_ + 1) % QN;
      count_--;
    }
    q_[(head_ + count_) % QN] = {cmd, arg};
    count_++;
  }

  // 7E FF 06 CMD 00 HI LO CKH CKL EF, checksum = -(sum of bytes 1..6)
  static void send(uint8_t cmd, uint16_t arg) {
    uint8_t f[10] = {0x7E, 0xFF, 0x06, cmd, 0x00, uint8_t(arg >> 8),
                     uint8_t(arg & 0xFF), 0, 0, 0xEF};
    uint16_t sum = 0;
    for (int i = 1; i < 7; i++) sum += f[i];
    uint16_t ck = uint16_t(0 - sum);
    f[7] = uint8_t(ck >> 8);
    f[8] = uint8_t(ck & 0xFF);
    DFPLAYER_SERIAL.write(f, sizeof(f));
  }

  // the SD card's 01/ folder (generated by tools/generate_voice_pack.py)
  static int trackFor(Cue c) {
    switch (c) {
      case Cue::Chime: return 1;
      case Cue::StartBell: return 2;
      case Cue::FinishBell: return 3;
      case Cue::BreatheIn: return 4;
      case Cue::BreatheHold: return 5;
      case Cue::BreatheOut: return 6;
      case Cue::Pop: return 7;
      case Cue::Crackle: return 8;
      case Cue::Eat: return 9;
      case Cue::Die: return 10;
      case Cue::Bounce: return 11;
      case Cue::Score: return 12;
      case Cue::Halfway: return 13;
      case Cue::Tick: return 14;
      default: return 0;
    }
  }
};

// ---- settings in the Teensy 4.1's emulated EEPROM ------------------------------
class EepromStore : public SettingsStore {
 public:
  // layout: magic, struct size, build stamp, Settings. A size mismatch
  // (the struct grew in an update) falls back to defaults rather than
  // reading garbage.
  bool load(Settings& out) {
    Header h;
    EEPROM.get(0, h);
    if (h.magic != MAGIC || h.size != sizeof(Settings)) return false;
    EEPROM.get(sizeof(Header), out);
    return true;
  }
  void save(const Settings& s) {
    Header h;
    EEPROM.get(0, h);
    uint32_t stamp = (h.magic == MAGIC) ? h.stamp : 0;
    h.magic = MAGIC;
    h.size = sizeof(Settings);
    h.stamp = stamp;
    EEPROM.put(0, h);
    EEPROM.put(sizeof(Header), s);
  }
  // true once per flashed build: the first boot after an upload
  bool firstBootOfBuild() {
    Header h;
    EEPROM.get(0, h);
    uint32_t stamp = buildStamp();
    if (h.magic == MAGIC && h.stamp == stamp) return false;
    h.magic = MAGIC;
    h.stamp = stamp;
    EEPROM.put(0, h);
    return true;
  }

 private:
  struct Header {
    uint32_t magic;
    uint32_t size;
    uint32_t stamp;
  };
  static const uint32_t MAGIC = 0xC0410126;  // "Coriolis", layout v2
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
// the three 48KB framebuffers (the scene canvas here, the shell's output and
// transition buffers) live in the second RAM bank, leaving the fast
// tightly-coupled RAM for code, stack and the driver
DMAMEM FrameBuffer fb;
Settings settings;
RtcTime timeSource(settings);
DeviceInput input;
DfPlayerAudio audio;
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

Scene* scenes[] = {&clockScene, &analogClock, &wordClock,    &weather,
                   &calendarScene, &pong,     &snake,        &tetris,
                   &labyrinth,  &yoga,        &exercise,     &breathe,
                   &focus,      &gifs,        &spiro,        &mandala,
                   &coriolisScene, &lava,     &rain,         &bounce,
                   &starfield,  &life,        &aquarium,     &fire,
                   &aurora};
const int sceneCount = sizeof(scenes) / sizeof(scenes[0]);

Context ctx = {fb, timeSource, input, audio, &palettes::byIndex(0), 0};
DMAMEM Shell shell(scenes, sceneCount, settings, settingsScene);

void DeviceInput::jumpToDigit(Shell& sh, Context& c, int digit) {
  static const char* favourites[9] = {"Clock",  "Analog",   "Word Clock",
                                      "Weather", "Calendar", "Yoga",
                                      "Exercise", "Breathe", "Fireplace"};
  for (int i = 0; i < sceneCount; i++)
    if (strcmp(scenes[i]->name(), favourites[digit - 1]) == 0) sh.show(c, i);
}

time_t getRtcTime() { return Teensy3Clock.get(); }

// four quadrants, numbered, with an arrow in the top-left corner: shows at a
// glance whether the panel chain and stacking options are right
void drawTestPattern(FrameBuffer& f) {
  f.clear();
  const RGB q[4] = {RGB(200, 40, 40), RGB(40, 200, 40), RGB(40, 80, 220),
                    RGB(200, 200, 40)};
  const int W = f.width(), H = f.height();
  for (int i = 0; i < 4; i++) {
    int x0 = (i % 2) * W / 2, y0 = (i / 2) * H / 2;
    for (int k = 0; k < W / 2; k++) {
      f.set(x0 + k, y0, q[i]);
      f.set(x0 + k, y0 + H / 2 - 1, q[i]);
      f.set(x0, y0 + k, q[i]);
      f.set(x0 + W / 2 - 1, y0 + k, q[i]);
    }
    char n[2] = {char('1' + i), 0};
    font5x7::drawCentered(f, n, x0 + W / 4, y0 + H / 4 - 14, 4, q[i]);
  }
  for (int k = 0; k < 12; k++) {  // arrow pointing at the top-left corner
    f.set(3 + k, 3, RGB(255, 255, 255));
    f.set(3, 3 + k, RGB(255, 255, 255));
    f.set(3 + k, 3 + k, RGB(255, 255, 255));
  }
}

void blit(const FrameBuffer& source) {
  // copy the composed frame to the panel, applying rotation; brightness is
  // handled globally by the matrix driver
  rgb24* out = backgroundLayer.backBuffer();
  const int W = source.width(), H = source.height();
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      const RGB& c = source.at(x, y);
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

void setup() {
  Serial.begin(115200);
  setSyncProvider(getRtcTime);  // TimeLib follows the hardware RTC
  IrReceiver::begin(kIrPin);
  audio.begin();

  store.load(settings);
  if (store.firstBootOfBuild()) {
    timeSource.adoptLoaderTime();
    Serial.println("[time] first boot of this build: RTC converted to UTC");
  }

  matrix.addLayer(&backgroundLayer);
  matrix.begin();
  matrix.setBrightness(min(int(settings.brightness), CORIOLIS_MAX_BRIGHTNESS));
  matrix.setRefreshRate(120);

  ctx.nowMs = millis();
  shell.begin(ctx);
}

void loop() {
  ctx.nowMs = millis();
  input.poll(shell, ctx, timeSource);

  if (input.testPattern) {
    static uint32_t lastMs = 0;
    if (ctx.nowMs - lastMs > 200) {
      lastMs = ctx.nowMs;
      drawTestPattern(fb);
      matrix.setBrightness(min(96, CORIOLIS_MAX_BRIGHTNESS));
      blit(fb);
    }
    return;
  }

  if (shell.tick(ctx)) {
    matrix.setBrightness(min(int(shell.brightness(ctx)), CORIOLIS_MAX_BRIGHTNESS));
    blit(shell.frame());
  }
}
