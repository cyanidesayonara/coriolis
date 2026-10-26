# Audio & DFPlayer Mini Guide

Audio in Coriolis is an interface (`core/audio.h`), like time and input. Scenes fire named
**cues** and speak **voice clips** by number; backends turn those into sound:

- **Simulator**: `SimAudio` in `sim/main.cpp` synthesizes short beeps (toggle with `A`, off by default).
- **Device**: `DfPlayerAudio` in `firmware/coriolis/coriolis.ino` drives a **DFPlayer Mini** on
  `Serial1` (Teensy pins 0/1), which plays numbered clips from its own microSD card. No network.

The firmware queues commands (the module takes one at a time, ~60 ms apart), lets a chime ring
before a voice clip fired in the same moment, and while a loop plays (the fireplace crackle) sends
one-shots as DFPlayer *advertisements*, which pause the loop, play, and resume it.

---

## 1. Generating the sound pack

```bash
python tools/generate_voice_pack.py                      # into audio_pack/
python tools/generate_voice_pack.py --voice en-US-JennyNeural
```

Chimes are synthesized as 16-bit WAV. Speech uses Microsoft Edge neural TTS (`pip install edge-tts`)
and falls back to Windows SAPI (WAV) without it. Output:

```
audio_pack/
├── 01/       cues 001..014
├── 02/       voice clips 001..247 (numbers below)
└── ADVERT/   the cues again as 0001..0014, for playing over a loop
```

## 2. microSD card

FAT32 (or FAT16), up to 32 GB. Copy `01`, `02` and `ADVERT` to the **root**. Don't rename files:
the DFPlayer finds tracks by folder number and the leading digits of the file name.

## 3. Cues (folder `01`)

`ctx.audio.play(Cue)` / `ctx.audio.loop(Cue)` — the track numbers are fixed in `DfPlayerAudio::trackFor()`.

| Cue | Track | Sound | Fired by |
|-----|-------|-------|----------|
| `Chime` | 001 | Soft C-major chord | Pose / move change, Tetris line |
| `StartBell` | 002 | Rising C-E-G | Session / game starts |
| `FinishBell` | 003 | Falling cascade | Move or session complete |
| `BreatheIn` | 004 | Rising tone | Breathe: inhale |
| `BreatheHold` | 005 | Calm hum | Breathe: hold |
| `BreatheOut` | 006 | Falling tone | Breathe: exhale |
| `Pop` | 007 | Spark pop | Fireplace, calendar fireworks |
| `Crackle` | 008 | Crackle (looped) | Fireplace ambient bed |
| `Eat` | 009 | 8-bit blip | Snake eats |
| `Die` | 010 | Falling tone | Game over |
| `Bounce` | 011 | Paddle click | Pong |
| `Score` | 012 | Win chime | Pong / Labyrinth |
| `Halfway` | 013 | Double chime | Halfway through a long hold or set |
| `Tick` | 014 | Woodblock tick | 3-2-1 countdowns, last seconds of holds/rests |

## 4. Voice clips (folder `02`)

`ctx.audio.voice(n)` plays `02/nnn` — **the number is the file name**. The ranges are constants in
`core/audio.h` (`namespace voice`) and must stay ≤ 255 (the DFPlayer's per-folder limit).

| Range | Content |
|---|---|
| 001–019 | Yoga poses: 1 Mountain · 2 Star · 3 Warrior two · 4 Triangle · 5 Tree · 6 Chair · 7 Forward fold · 8 Downward dog · 9 Cobra · 10 Child's pose · 11 Plank · 12 Warrior one · 13 Bridge · 14 Savasana · 15 Cat · 16 Cow · 17 Low lunge · 18 Pigeon · 19 Boat |
| 101–116 | Exercise moves: 101 Squats · 102 Push-ups · 103 Sit-ups · 104 Lunges · 105 Jumping jacks · 106 Kettlebell swings · 107 Goblet squats · 108 Kettlebell rows · 109 Kettlebell press · 113 Glute bridges · 114 Plank hold · 115 Mountain climbers · 116 High knees |
| 201–230 | Numbers “one” … “thirty” (rep counting: `200 + n`) |
| 241–247 | Prompts: 241 Switch sides · 242 Halfway there · 243 Workout complete, great job · 244 Get ready · 245 Namaste · 246 Work · 247 Rest |

`tests/test_core.cpp` runs every yoga flow and exercise program and checks that all clip numbers
stay in range; the generator's table covers every number the scenes use.

## 5. Wiring (Teensy 4.1 ↔ DFPlayer Mini)

| DFPlayer | Connects to | Notes |
|---|---|---|
| VCC | 5 V rail | from the PSU, not through the Teensy |
| GND | common ground | |
| TX | Teensy pin 0 (RX1) | 3.3 V logic, safe for the Teensy |
| RX | Teensy pin 1 (TX1) | through a **1 kΩ** series resistor |
| SPK_1 / SPK_2 | speaker | 3 W, 4–8 Ω; neither terminal to ground |

Serial2 (pins 7/8) is not usable: pin 7 is the SmartLED Shield's panel clock.
