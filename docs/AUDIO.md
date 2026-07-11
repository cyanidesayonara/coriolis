# Audio

Audio is an interface (`core/audio.h`), like time and input. Scenes fire named
**cues** and speak **voice clips** by index; a backend turns those into sound.
The display itself stays fully offline — all audio is local files.

- **Simulator**: `SimAudio` in `sim/main.cpp` synthesizes short beeps (toggle
  with `A`, off by default). Voice clips play a neutral blip.
- **Device**: a **DFPlayer Mini** (~€5 serial MP3 module + small speaker, its
  own microSD) plays numbered clips. One UART, keeps everything offline.

## Cues → clips (folder `01` on the SD card)

`AudioSink::play(Cue)` and `AudioSink::loop(Cue)`:

| Cue | Track | Sound | Fired by |
|-----|-------|-------|----------|
| `Chime` | `01/001.mp3` | soft chime | yoga/exercise step change |
| `StartBell` | `01/002.mp3` | rising bell | a session begins |
| `FinishBell` | `01/003.mp3` | falling bell | a move/session ends |
| `BreatheIn` | `01/004.mp3` | rising tone | breathe: inhale |
| `BreatheHold` | `01/005.mp3` | short tone | breathe: hold |
| `BreatheOut` | `01/006.mp3` | falling tone | breathe: exhale |
| `Pop` | `01/007.mp3` | ember pop | fireplace spark burst |
| `Crackle` | `01/008.mp3` | crackle loop | fireplace ambient bed (`loop`) |
| `Eat` | `01/009.mp3` | blip | snake eats |
| `Die` | `01/010.mp3` | descending | snake / game over |
| `Bounce` | `01/011.mp3` | click | pong paddle/wall |
| `Score` | `01/012.mp3` | tone | pong point |

`loop(Cue::Crackle)` starts the ambient bed; `loop(Cue::None)` stops it.

## Voice clips (folder `02`) — `AudioSink::voice(index)`

Spoken words, generated once with any TTS and copied to the card. The index is
namespaced so scenes don't collide:

| Index range | Meaning | Clips |
|-------------|---------|-------|
| `0..19` | yoga pose names (0=Mountain … 5=Cobra) | `02/001.mp3` = index 0, … |
| `100..119` | exercise move names (100=first move …) | `02/101.mp3` = index 100, … |
| `200..231` | spoken numbers (`200 + n` = "n") | `02/201.mp3` = "one", … |

Device mapping is simply `file = 02/(index + 1).mp3` within each block, or a
small lookup — whatever the DFPlayer library makes convenient. The ranges are
the contract; the exact filenames are the backend's business.

## Device backend (later)

A `DFPlayerAudio : AudioSink` in `firmware/` wraps the DFPlayerMini library:
`play(Cue)` → `playFolder(1, cueTrack)`, `voice(i)` → `playFolder(2, ...)`,
`loop(Cue::Crackle)` → `loopFolder`/`enableLoop`. Keep a tiny queue so a voice
clip doesn't cut off a chime. The DFPlayer has its own volume; the display's
brightness setting does not affect it.
