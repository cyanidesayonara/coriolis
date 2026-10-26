# IR Remote Guide

Coriolis is an appliance: no app, no Wi-Fi. A handheld infrared remote is the interface. The
decoder is in `firmware/coriolis/coriolis.ino` (`IrReceiver`, `RemoteMap`, `DeviceInput`); it
turns buttons into the same keys and commands the simulator's keyboard produces, and the shared
`Shell` (`scenes/shell.h`) does the rest.

## 1. Supported remotes

- **Apple Remote** — aluminium A1294 (7 buttons) or white A1156. NEC timing, Apple custom code
  `0xEE 0x87`; the command byte's lowest bit is parity and is ignored.
- **17-key NEC “Arduino kit” remote** — address `0x00`, arrows, OK, digits, `*`, `#`.

Any other NEC remote can be added: every received frame is printed on USB serial as
`[IR] 0xXXXXXXXX -> n` (n = 0: not mapped). Add its command bytes in `RemoteMap::decode()`.

## 2. Receiver and wiring

| Part | Notes |
|---|---|
| Vishay **TSOP38438** (or TSOP38238), 38 kHz | output is active-low with an internal pull-up |
| 100 Ω + 10 µF | RC supply filter right at the sensor, against panel PWM noise |

TSOP pins, lens facing you: **OUT · GND · Vs**.

- Vs → 100 Ω → **3.3 V** (never 5 V: the Teensy 4.1 isn't 5 V tolerant), 10 µF from Vs to GND
- GND → GND
- OUT → **Teensy pin 16**

> Earlier versions of this guide used pin 2. On the SmartLED Shield V5, **pin 2 is the panels'
> OE line** — the receiver must not go there.

## 3. Button map

| Apple Remote | 17-key remote | Tap | Hold ~1 s |
|---|---|---|---|
| Up / Down / Left / Right | arrows | navigation keys (held: games keep steering, menus auto-repeat) | |
| Center | OK | OK / select / pause | |
| **Menu** | `*` | scene launcher; closes launcher or settings | **Settings** |
| **Play/Pause** | — | same as OK | **Home** (back to the clock) |
| — | `#` | Back | |
| — | `0` | clock overlay type | |
| — | `1`…`9` | Clock, Analog, Word clock, Weather, Calendar, Yoga, Exercise, Breathe, Fireplace | |

Menu and Play/Pause act on release, so a hold can mean something else (≈150 ms later than a tap).

What the keys do in each context (launcher, settings, workouts, games) is in the user's manual,
[`Coriolis_User_Manual.pdf`](Coriolis_User_Manual.pdf).

## 4. Command codes

Apple (`b0 = 0xEE`, `b1 = 0x87`, command `b2 & 0xFE`):

| Button | Command |
|---|---|
| Up | 0x0A |
| Down | 0x0C |
| Left | 0x08 |
| Right | 0x06 |
| Center (aluminium) | 0x5C |
| Center/Play (white) | 0x04 |
| Menu | 0x02 |
| Play/Pause (aluminium) | 0x5E |

17-key NEC (address 0x00, `b3 = ~b2`): Up 0x18 · Down 0x52 · Left 0x08 · Right 0x5A · OK 0x1C ·
`*` 0x16 · `#` 0x0D · 0 0x19 · 1 0x45 · 2 0x46 · 3 0x47 · 4 0x44 · 5 0x40 · 6 0x43 · 7 0x07 ·
8 0x15 · 9 0x09.

Sources disagree on byte order for Apple remotes as other decoders report them; this decoder
assembles bits LSB-first exactly as transmitted. If your remote doesn't respond, check the
`[IR]` serial lines first.

## 5. Bench testing

Open the serial monitor at 115200 baud. Every frame prints its raw code. The keyboard also works
over serial: `u d l r` arrows, `k` OK, `b` back, `m` menu, `s` settings, `c` overlay, `n` next,
`h` home, `t` panel test pattern, `i` info.
