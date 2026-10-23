# Audio & DFPlayer Mini Guide

Audio in Coriolis is an interface (`core/audio.h`), like time and input. Scenes fire named
**cues** and speak **voice clips** by index; backends turn those into sound:

- **Simulator**: `SimAudio` in `sim/main.cpp` synthesizes short beeps and cues (toggle with `A`, off by default).
- **Device**: A **DFPlayer Mini** (~€5 serial MP3 module with on-board DAC and 3W speaker output, connected to a microSD card) plays offline numbered clips. Zero Wi-Fi or cloud dependencies.

---

## 1. Generating the Voice & Sound Pack

A turnkey generator script is provided in `tools/generate_voice_pack.py`. It synthesizes all cues and uses Microsoft Edge Neural TTS (or local Windows SAPI fallback) to generate natural, studio-quality speech clips:

```bash
# Generate the full sound pack into audio_pack/
python tools/generate_voice_pack.py

# Or choose a specific neural voice:
python tools/generate_voice_pack.py --voice en-US-AriaNeural
```

The script produces two numbered folders ready for the microSD card:
- `audio_pack/01/`: System cues & sound effects (`001.wav` .. `014.wav`)
- `audio_pack/02/`: Spoken voice guidance clips (`001.mp3` .. `257.mp3`)

---

## 2. MicroSD Card Preparation

1. **Card Size**: MicroSD card up to 32 GB.
2. **File System**: Format as **FAT32** (or FAT16) with standard 32KB allocation unit size.
3. **Card Structure**: Copy `01` and `02` directly to the root of the microSD card:
   ```
   SD_CARD/
   ├── 01/
   │   ├── 001.wav  (Chime)
   │   ├── 002.wav  (StartBell)
   │   ├── ...
   │   └── 014.wav  (Tick)
   └── 02/
       ├── 001.mp3  (Mountain Pose)
       ├── 101.mp3  (Squats)
       ├── 201.mp3  (One)
       └── 251.mp3  (Switch sides)
   ```
   > [!IMPORTANT]
   > Do not rename files or alter the leading three-digit numbering format (`001`, `002`...). DFPlayer Mini's firmware indexes tracks based on folder and numerical filename prefixes.

---

## 3. Cues Index (Folder `01`)

Triggered via `ctx.audio.play(Cue)` and `ctx.audio.loop(Cue)`:

| Cue | Track | Sound | Fired by |
|-----|-------|-------|----------|
| `Chime` | `01/001.wav` | Soft C-major harmonic chord | Yoga/exercise step change |
| `StartBell` | `01/002.wav` | Rising triple chime (C5-E5-G5) | Session begins |
| `FinishBell` | `01/003.wav` | Celebratory descending cascade | Session completed |
| `BreatheIn` | `01/004.wav` | Swelling rising tone (220→440 Hz) | Breathe: inhale |
| `BreatheHold` | `01/005.wav` | Gentle calming harmonic hum | Breathe: hold |
| `BreatheOut` | `01/006.wav` | Relaxing falling tone (440→220 Hz) | Breathe: exhale |
| `Pop` | `01/007.wav` | Fireplace spark burst | Fireplace ambient spark |
| `Crackle` | `01/008.wav` | Ambient crackle loop | Fireplace ambient bed (`loop`) |
| `Eat` | `01/009.wav` | High 8-bit blip | Snake eats food |
| `Die` | `01/010.wav` | Descending low tone | Snake game over |
| `Bounce` | `01/011.wav` | Crisp paddle click | Pong bounce |
| `Score` | `01/012.wav` | High win chime | Pong point scored |
| `Halfway` | `01/013.wav` | Double chime | 50% hold elapsed in pose/set |
| `Tick` | `01/014.wav` | Woodblock countdown tick | 3-2-1 countdown tick |

---

## 4. Spoken Voice Index (Folder `02`)

Triggered via `ctx.audio.voice(index)`. The namespace layout guarantees zero collision across scenes:

### Yoga Poses (`0..19` → `02/001.mp3` .. `02/020.mp3`)
| Index | File | Spoken Phrase |
|-------|------|---------------|
| `0` | `02/001.mp3` | "Mountain Pose" |
| `1` | `02/002.mp3` | "Star Pose" |
| `2` | `02/003.mp3` | "Warrior Two" |
| `3` | `02/004.mp3` | "Triangle Pose" |
| `4` | `02/005.mp3` | "Tree Pose" |
| `5` | `02/006.mp3` | "Chair Pose" |
| `6` | `02/007.mp3` | "Forward Fold" |
| `7` | `02/008.mp3` | "Downward-Facing Dog" |
| `8` | `02/009.mp3` | "Cobra Pose" |
| `9` | `02/010.mp3` | "Child's Pose" |
| `10` | `02/011.mp3` | "Plank" |
| `11` | `02/012.mp3` | "Warrior One" |
| `12` | `02/013.mp3` | "Bridge Pose" |
| `13` | `02/014.mp3` | "Savasana" |
| `14` | `02/015.mp3` | "Cat Pose" |
| `15` | `02/016.mp3` | "Cow Pose" |
| `16` | `02/017.mp3` | "Low Lunge" |
| `17` | `02/018.mp3` | "Pigeon Pose" |
| `18` | `02/019.mp3` | "Boat Pose" |

### Exercise Moves (`100..119` → `02/101.mp3` .. `02/120.mp3`)
| Index | File | Spoken Phrase |
|-------|------|---------------|
| `100` | `02/101.mp3` | "Squats" |
| `101` | `02/102.mp3` | "Push-ups" |
| `102` | `02/103.mp3` | "Sit-ups" |
| `103` | `02/104.mp3` | "Lunges" |
| `104` | `02/105.mp3` | "Jumping Jacks" |
| `105` | `02/106.mp3` | "Kettlebell Swings" |
| `106` | `02/107.mp3` | "Goblet Squats" |
| `107` | `02/108.mp3` | "Kettlebell Rows" |
| `108` | `02/109.mp3` | "Kettlebell Press" |
| `109` | `02/110.mp3` | "Tabata Sprint" |
| `110` | `02/111.mp3` | "Core and Pilates" |
| `111` | `02/112.mp3` | "Rest" |

### Spoken Numbers (`200..230` → `02/201.mp3` .. `02/231.mp3`)
Spoken reps `1` to `30` ("One" .. "Thirty") corresponding to `200 + rep`.

### Coaching Prompts (`250..260` → `02/251.mp3` .. `02/260.mp3`)
| Index | File | Spoken Phrase |
|-------|------|---------------|
| `250` | `02/251.mp3` | "Switch sides" |
| `251` | `02/252.mp3` | "Halfway there" |
| `252` | `02/253.mp3` | "Workout complete! Great job." |
| `253` | `02/254.mp3` | "Three" |
| `254` | `02/255.mp3` | "Two" |
| `255` | `02/256.mp3` | "One" |
| `256` | `02/257.mp3` | "Get ready" |

---

## 5. Hardware Wiring (Teensy 4.1 ↔ DFPlayer Mini)

| DFPlayer Pin | Label | Teensy 4.1 Pin | Description |
|:---:|:---:|:---:|:---|
| 1 | `VCC` | `VIN` (5V) | 5V Power from display supply |
| 2 | `RX` | `Pin 1` (`TX1`) | Serial receive from Teensy (put a 1kΩ resistor in series) |
| 3 | `TX` | `Pin 0` (`RX1`) | Serial transmit to Teensy |
| 6 | `SPK_1` | Speaker (+) | Direct 3W speaker lead |
| 8 | `SPK_2` | Speaker (-) | Direct 3W speaker lead |
| 7 / 10 | `GND` | `GND` | Common Ground |

> [!TIP]
> Always place a 1kΩ resistor in series between Teensy 4.1 TX (3.3V) and DFPlayer RX (5V) to eliminate UART line noise and ground hum.
