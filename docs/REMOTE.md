# IR Remote Control Guide — Coriolis

Coriolis is designed as an appliance for the living room: no phone apps, no WiFi dependencies, and no laggy menus. A physical handheld remote control is the primary interface for switching scenes, tuning ambient art, controlling retro games, and pacing guided workouts from the yoga mat.

---

## 1. Recommended Remote Hardware

The cheap 24-key plastic credit-card remotes feel mushy and have weak transmitters that don't reach across a living room. For a permanent handcrafted wall installation, an **aluminum unibody NEC IR remote** fits the aesthetic of high-end home audio equipment:

* **Primary Choice**: **Apple Remote A1294** (or third-party aluminum unibody replacement).
  * Solid bead-blasted aluminum, balanced weight, crisp tactile click switches.
  * Transmitter: Strong 38 kHz infrared emitter with wide-angle spread.
  * Clean, minimal 7-button layout:
    * Directional Ring: `Up`, `Down`, `Left`, `Right`
    * Center: `Select / OK`
    * Lower Buttons: `Menu / Back`, `Play / Pause`
  * Protocol: Standard NEC pulse-distance encoding (38 kHz carrier, Apple vendor ID `0xEE87`).
* **Alternative Universal Remotes**:
  * Any minimalist NEC-protocol remote (SofaBaton, Satechi aluminum, or Chunghop universal programmed to NEC code 0001).

---

## 2. Sensor Hardware & Wiring

### Bill of Materials

| Part | Spec | Notes |
|---|---|---|
| **IR Receiver** | TSOP38238 or TSOP4838 | 38 kHz carrier frequency, active-low output |
| **Decoupling Resistor** | 100 Ω | Filters power supply ripple from matrix panels |
| **Decoupling Capacitor** | 4.7 µF to 10 µF electrolytic | Places between Vs and GND at the sensor |

### Pinout (TSOP38238, front facing you — lens bump toward you)

```
       _____
      /     \
     |  (o)  |    (Lens facing front)
     |_______|
       | | |
       1 2 3
       | | |
       | | +-- Pin 3: Vs  (+3.3V via 100Ω resistor)
       | +---- Pin 2: GND (Ground)
       +------ Pin 1: OUT (Data signal to Teensy 4.1 Pin 2)
```

### Teensy 4.1 Connection

The SmartLED Shield V5 leaves several high-speed digital pins open on the Teensy 4.1 outer header:

* **TSOP Pin 1 (OUT)** $\to$ **Teensy 4.1 Pin 2** (Interrupt-capable)
* **TSOP Pin 2 (GND)** $\to$ **Teensy GND**
* **TSOP Pin 3 (Vs)** $\to$ **100Ω Resistor** $\to$ **Teensy 3.3V**
* Place the **4.7µF capacitor** directly across TSOP Pin 2 (GND) and Pin 3 (Vs) to prevent LED panel PWM noise from triggering false signals.

---

## 3. Remote Key Mappings

### General Navigation (Wall Clock & Ambient Art)

| Remote Button | Action |
|---|---|
| **Right (`→`)** | Next scene |
| **Left (`←`)** | Previous scene |
| **Up (`↑`)** | Cycle color palette forward |
| **Down (`↓`)** | Cycle color palette backward |
| **Center (`OK`)** | Contextual action (start game, select item) |
| **Menu** *(short press)* | **Scene Launcher** — categorized quick-jump grid |
| **Menu** *(long press)* | **Settings Menu** — fine-tuning and preferences |
| **Play / Pause** | Toggle active animation, timer, or sound |

### Guided Fitness Suite (Yoga & Kettlebell Workouts)

When stepping onto the mat, the remote acts as your hands-free workout coach:

| Remote Button | Workout Action |
|---|---|
| **Play / Pause** | Freeze / unfreeze pose hold or interval timer (take a water break) |
| **Right (`→`)** | Skip immediately to the next pose/exercise |
| **Left (`←`)** | Jump back to redo the previous pose |
| **Up (`↑`)** | Increase hold duration (+5 seconds) |
| **Down (`↓`)** | Decrease hold duration (-5 seconds) |
| **Center (`OK`)** | Start routine (from setup card or completion screen) |
| **Menu** | Return to routine setup or exit to clock |

### Scene Launcher Navigation

When `Menu` is tapped:

* **Up / Down**: Switch category (*Clocks & Info*, *Guided Fitness*, *Games*, *Ambient Art*)
* **Left / Right**: Highlight scene in category
* **Center (`OK`)**: Jump directly to selected scene
* **Menu**: Close launcher without changing scene

---

## 4. NEC Protocol & Code Reference

The firmware decodes standard NEC 38kHz pulses via hardware interrupt on Pin 2.

### Apple A1294 NEC Codes (Command Byte)

The Apple remote transmits a 32-bit frame:
`[Vendor ID 16-bit: 0xEE87] [Command 8-bit] [Device ID 8-bit]`

| Button | Hex Command |
|---|---|
| **Up** | `0x0B` (repeat `0x50`) |
| **Down** | `0x0D` (repeat `0x30`) |
| **Left** | `0x08` (repeat `0x90`) |
| **Right** | `0x07` (repeat `0x60`) |
| **Select / Center** | `0x04` (alternate `0x5C`) |
| **Menu** | `0x02` (repeat `0xC0`) |
| **Play / Pause** | `0x01` (alternate `0x5E`) |

### Standard Universal NEC Codes (Address `0x00FF` or `0x00`)

| Button | Hex Command |
|---|---|
| **Up** | `0x18` or `0x40` |
| **Down** | `0x52` or `0x19` |
| **Left** | `0x08` or `0x07` |
| **Right** | `0x5A` or `0x09` |
| **Select / OK** | `0x1C` or `0x15` |
| **Menu / Back** | `0x45` or `0x44` |
| **Play / Pause** | `0x46` or `0x47` |

---

## 5. Bench Testing with Serial Monitor

Before mounting the display into its frame, test the IR receiver on the desk:

1. Flash the firmware (`arduino-cli compile --fqbn teensy:avr:teensy41 firmware/coriolis`).
2. Open USB Serial at 115200 baud (`arduino-cli monitor -p COMx -c baudrate=115200`).
3. Press any button on the remote:
   ```
   [IR] NEC Code: 0x07 (Right) -> Next Scene
   [IR] NEC Code: 0x02 (Menu)  -> Open Launcher
   ```
4. Both USB serial keystrokes (`u`, `d`, `l`, `r`, `k`, `b`, `m`, `s`) and physical IR remote pulses operate simultaneously.
