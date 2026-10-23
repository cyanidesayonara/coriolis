#!/usr/bin/env python3
"""
Coriolis DFPlayer Mini Voice & Audio Pack Generator
===================================================

Generates the complete set of offline audio clips (sound effects, yoga poses,
exercise moves, and spoken numbers) for the DFPlayer Mini MP3 hardware module
on the Coriolis matrix clock.

DFPlayer Mini SD Card Layout:
  SD_CARD/
    01/  -> System cues & sound effects (001.mp3 .. 014.mp3)
    02/  -> Voice clips: poses, exercises, numbers (001.mp3 .. 253.mp3)

Usage:
  python tools/generate_voice_pack.py [--out-dir audio_pack] [--voice en-US-AriaNeural]
"""

import os
import sys
import math
import wave
import struct
import asyncio
import argparse

# Check if edge-tts is available
try:
    import edge_tts
    HAS_EDGE_TTS = True
except ImportError:
    HAS_EDGE_TTS = False


# ---------------------------------------------------------------------------
# Cues Mapping (Folder 01)
# ---------------------------------------------------------------------------
CUES = [
    (1, "Chime", "Soft chime for pose and step transitions"),
    (2, "StartBell", "Rising session start bell"),
    (3, "FinishBell", "Falling celebratory session complete bell"),
    (4, "BreatheIn", "Rising inhalation tone"),
    (5, "BreatheHold", "Gentle hold tone"),
    (6, "BreatheOut", "Falling exhalation tone"),
    (7, "Pop", "Ember pop for fireplace"),
    (8, "Crackle", "Ambient fireplace crackle"),
    (9, "Eat", "Snake food pickup blip"),
    (10, "Die", "Game over descending sound"),
    (11, "Bounce", "Pong paddle bounce click"),
    (12, "Score", "Pong point score tone"),
    (13, "Halfway", "Halfway through pose hold chime"),
    (14, "Tick", "3-2-1 countdown tick"),
]

# ---------------------------------------------------------------------------
# Voice Clips Mapping (Folder 02)
# Range 0..19: Yoga Poses (02/001.mp3 .. 02/020.mp3)
# Range 100..119: Exercise Moves (02/101.mp3 .. 02/120.mp3)
# Range 200..231: Spoken Numbers (02/201.mp3 .. 02/231.mp3)
# Range 250..260: Guidance Cues (02/251.mp3 .. 02/260.mp3)
# ---------------------------------------------------------------------------
VOICE_CLIPS = {
    # Yoga poses (Index 0..13)
    1: "Mountain Pose",
    2: "Star Pose",
    3: "Warrior Two",
    4: "Triangle Pose",
    5: "Tree Pose",
    6: "Chair Pose",
    7: "Forward Fold",
    8: "Downward-Facing Dog",
    9: "Cobra Pose",
    10: "Child's Pose",
    11: "Plank",
    12: "Warrior One",
    13: "Bridge Pose",
    14: "Savasana",
    15: "Cat Pose",
    16: "Cow Pose",
    17: "Low Lunge",
    18: "Pigeon Pose",
    19: "Boat Pose",

    # Exercise moves (Index 100..111)
    101: "Squats",
    102: "Push-ups",
    103: "Sit-ups",
    104: "Lunges",
    105: "Jumping Jacks",
    106: "Kettlebell Swings",
    107: "Goblet Squats",
    108: "Kettlebell Rows",
    109: "Kettlebell Press",
    110: "Tabata Sprint",
    111: "Core and Pilates",
    112: "Rest",

    # Spoken numbers (Index 201..230)
    201: "One",
    202: "Two",
    203: "Three",
    204: "Four",
    205: "Five",
    206: "Six",
    207: "Seven",
    208: "Eight",
    209: "Nine",
    210: "Ten",
    211: "Eleven",
    212: "Twelve",
    213: "Thirteen",
    214: "Fourteen",
    215: "Fifteen",
    216: "Sixteen",
    217: "Seventeen",
    218: "Eighteen",
    219: "Nineteen",
    220: "Twenty",
    221: "Twenty-one",
    222: "Twenty-two",
    223: "Twenty-three",
    224: "Twenty-four",
    225: "Twenty-five",
    226: "Twenty-six",
    227: "Twenty-seven",
    228: "Twenty-eight",
    229: "Twenty-nine",
    230: "Thirty",

    # Coaching Cues (Index 250..255)
    251: "Switch sides",
    252: "Halfway there",
    253: "Workout complete! Great job.",
    254: "Three",
    255: "Two",
    256: "One",
    257: "Get ready",
}


# ---------------------------------------------------------------------------
# Synthetic PCM Cue Generator (16-bit 44.1kHz WAV)
# ---------------------------------------------------------------------------
def synthesize_cue_wav(track_num: int, filename: str):
    sample_rate = 44100

    def write_wav(samples):
        with wave.open(filename, 'w') as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(sample_rate)
            raw = bytearray()
            for s in samples:
                val = max(-32767, min(32767, int(s * 32767.0)))
                raw.extend(struct.pack('<h', val))
            wf.writeframes(raw)

    if track_num == 1:  # Chime: soft C-major chord (C5 + E5)
        duration = 0.8
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            env = math.exp(-4.5 * t)
            sig = 0.5 * math.sin(2 * math.pi * 523.25 * t) + \
                  0.35 * math.sin(2 * math.pi * 659.25 * t) + \
                  0.15 * math.sin(2 * math.pi * 1046.50 * t)
            samples.append(sig * env)
        write_wav(samples)

    elif track_num == 2:  # StartBell: rising arpeggio C5 -> E5 -> G5
        duration = 1.2
        total_samples = int(sample_rate * duration)
        samples = []
        notes = [(0.0, 523.25), (0.15, 659.25), (0.30, 783.99)]
        for i in range(total_samples):
            t = i / sample_rate
            sig = 0.0
            for start_t, freq in notes:
                if t >= start_t:
                    nt = t - start_t
                    env = math.exp(-3.5 * nt)
                    sig += 0.4 * math.sin(2 * math.pi * freq * nt) * env
            samples.append(sig)
        write_wav(samples)

    elif track_num == 3:  # FinishBell: celebratory falling chime
        duration = 1.5
        total_samples = int(sample_rate * duration)
        samples = []
        notes = [(0.0, 783.99), (0.18, 659.25), (0.36, 523.25), (0.54, 1046.50)]
        for i in range(total_samples):
            t = i / sample_rate
            sig = 0.0
            for start_t, freq in notes:
                if t >= start_t:
                    nt = t - start_t
                    env = math.exp(-3.0 * nt)
                    sig += 0.35 * math.sin(2 * math.pi * freq * nt) * env
            samples.append(sig)
        write_wav(samples)

    elif track_num == 4:  # BreatheIn: rising tone 220 -> 440 Hz
        duration = 1.0
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            freq = 220.0 + 220.0 * (t / duration)
            env = math.sin(math.pi * (t / duration))
            sig = 0.6 * math.sin(2 * math.pi * freq * t)
            samples.append(sig * env)
        write_wav(samples)

    elif track_num == 5:  # BreatheHold: steady calming tone 440 Hz
        duration = 0.6
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            env = math.sin(math.pi * (t / duration))
            sig = 0.5 * math.sin(2 * math.pi * 440.0 * t) + \
                  0.2 * math.sin(2 * math.pi * 880.0 * t)
            samples.append(sig * env)
        write_wav(samples)

    elif track_num == 6:  # BreatheOut: relaxing falling tone 440 -> 220 Hz
        duration = 1.0
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            freq = 440.0 - 220.0 * (t / duration)
            env = math.sin(math.pi * (t / duration))
            sig = 0.6 * math.sin(2 * math.pi * freq * t)
            samples.append(sig * env)
        write_wav(samples)

    elif track_num == 7:  # Pop: fireplace spark pop
        duration = 0.12
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            env = math.exp(-40.0 * t)
            sig = 0.7 * (math.sin(2 * math.pi * 1200.0 * t) +
                         math.sin(2 * math.pi * 2400.0 * t))
            samples.append(sig * env)
        write_wav(samples)

    elif track_num == 8:  # Crackle: gentle ambient burst
        duration = 0.4
        total_samples = int(sample_rate * duration)
        samples = []
        import random
        for i in range(total_samples):
            t = i / sample_rate
            noise = (random.random() * 2.0 - 1.0) * 0.3
            env = math.sin(math.pi * (t / duration))
            samples.append(noise * env)
        write_wav(samples)

    elif track_num == 9:  # Eat: retro game pickup blip
        duration = 0.1
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            freq = 987.77 if t < 0.05 else 1318.51
            sig = 0.6 if math.sin(2 * math.pi * freq * t) >= 0 else -0.6
            samples.append(sig)
        write_wav(samples)

    elif track_num == 10:  # Die: descending game over tone
        duration = 0.5
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            freq = max(80.0, 440.0 - 700.0 * t)
            env = 1.0 - (t / duration)
            sig = 0.6 if math.sin(2 * math.pi * freq * t) >= 0 else -0.6
            samples.append(sig * env)
        write_wav(samples)

    elif track_num == 11:  # Bounce: pong click
        duration = 0.04
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            sig = 0.7 * math.sin(2 * math.pi * 587.33 * t) * math.exp(-80.0 * t)
            samples.append(sig)
        write_wav(samples)

    elif track_num == 12:  # Score: pong score chime
        duration = 0.35
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            freq = 880.0 if t < 0.15 else 1174.66
            env = math.exp(-5.0 * (t % 0.15))
            sig = 0.6 * math.sin(2 * math.pi * freq * t) * env
            samples.append(sig)
        write_wav(samples)

    elif track_num == 13:  # Halfway: double harmonic chime
        duration = 0.5
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            sig = 0.0
            if t < 0.2:
                sig = 0.5 * math.sin(2 * math.pi * 659.25 * t) * math.exp(-8.0 * t)
            elif t >= 0.2:
                nt = t - 0.2
                sig = 0.6 * math.sin(2 * math.pi * 880.00 * nt) * math.exp(-8.0 * nt)
            samples.append(sig)
        write_wav(samples)

    elif track_num == 14:  # Tick: woodblock countdown click
        duration = 0.05
        total_samples = int(sample_rate * duration)
        samples = []
        for i in range(total_samples):
            t = i / sample_rate
            sig = 0.8 * math.sin(2 * math.pi * 1200.0 * t) * math.exp(-120.0 * t)
            samples.append(sig)
        write_wav(samples)


# ---------------------------------------------------------------------------
# TTS Generator
# ---------------------------------------------------------------------------
async def generate_voice_edge(text: str, voice: str, out_path: str):
    """Generate high-quality MP3 using Microsoft Edge neural TTS."""
    communicate = edge_tts.Communicate(text, voice)
    await communicate.save(out_path)


def generate_voice_sapi(text: str, out_path: str):
    """Fallback Windows SAPI synthesizer to WAV."""
    import subprocess
    cmd = [
        "powershell", "-NoProfile", "-Command",
        f"Add-Type -AssemblyName System.Speech; "
        f"$s = New-Object System.Speech.Synthesis.SpeechSynthesizer; "
        f"$s.SetOutputToWaveFile('{out_path}'); "
        f"$s.Speak('{text}'); "
        f"$s.Dispose()"
    ]
    subprocess.run(cmd, check=True)


# ---------------------------------------------------------------------------
# Main Orchestration
# ---------------------------------------------------------------------------
async def main():
    parser = argparse.ArgumentParser(description="Generate Coriolis DFPlayer Mini Voice Pack")
    parser.add_argument("--out-dir", default="audio_pack", help="Destination folder for SD card files")
    parser.add_argument("--voice", default="en-US-AriaNeural", help="Edge TTS voice (e.g. en-US-AriaNeural, en-US-JennyNeural)")
    parser.add_argument("--force-sapi", action="store_true", help="Force local Windows SAPI fallback")
    args = parser.parse_args()

    out_dir = os.path.abspath(args.out_dir)
    cues_dir = os.path.join(out_dir, "01")
    voice_dir = os.path.join(out_dir, "02")
    os.makedirs(cues_dir, exist_ok=True)
    os.makedirs(voice_dir, exist_ok=True)

    use_edge = HAS_EDGE_TTS and not args.force_sapi

    print("=" * 65)
    print(" Coriolis DFPlayer Mini Voice Pack Generator")
    print("=" * 65)
    print(f"Output directory : {out_dir}")
    print(f"TTS Engine       : {'Microsoft Edge Neural TTS (' + args.voice + ')' if use_edge else 'Windows SAPI'}")
    print()

    # 1. Generate Cues (Folder 01)
    print("Generating cues in folder 01/...")
    for track_num, name, desc in CUES:
        wav_path = os.path.join(cues_dir, f"{track_num:03d}.wav")
        synthesize_cue_wav(track_num, wav_path)
        print(f"  [01/{track_num:03d}.wav] {name:12s} - {desc}")

    print("\nGenerating voice clips in folder 02/...")
    total = len(VOICE_CLIPS)
    count = 0
    for num, text in sorted(VOICE_CLIPS.items()):
        count += 1
        ext = ".mp3" if use_edge else ".wav"
        target_path = os.path.join(voice_dir, f"{num:03d}{ext}")

        print(f"  ({count:2d}/{total:2d}) [02/{num:03d}{ext}] \"{text}\"")
        if use_edge:
            try:
                await generate_voice_edge(text, args.voice, target_path)
            except Exception as e:
                print(f"    Warning: Edge TTS failed ({e}), falling back to SAPI...")
                wav_target = os.path.join(voice_dir, f"{num:03d}.wav")
                generate_voice_sapi(text, wav_target)
        else:
            generate_voice_sapi(text, target_path)

    # 2. Write README.txt for the SD Card
    readme_path = os.path.join(out_dir, "README.txt")
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(
            "Coriolis LED Matrix Display — DFPlayer Mini MicroSD Card\n"
            "=======================================================\n\n"
            "MicroSD Card Requirements:\n"
            "  1. Format card as FAT32 (or FAT16), 32GB maximum.\n"
            "  2. Copy the folders '01' and '02' directly to the root of the microSD card.\n"
            "  3. Do not rename the folders or file numbers.\n\n"
            "Folder Layout:\n"
            "  /01/  -> Cues and sound effects\n"
            "           001.wav/mp3 : Chime (step/pose change)\n"
            "           002.wav/mp3 : StartBell (session starts)\n"
            "           003.wav/mp3 : FinishBell (session ends)\n"
            "           004.wav/mp3 : BreatheIn\n"
            "           005.wav/mp3 : BreatheHold\n"
            "           006.wav/mp3 : BreatheOut\n"
            "           007.wav/mp3 : Pop\n"
            "           008.wav/mp3 : Crackle\n"
            "           009.wav/mp3 : Eat\n"
            "           010.wav/mp3 : Die\n"
            "           011.wav/mp3 : Bounce\n"
            "           012.wav/mp3 : Score\n"
            "           013.wav/mp3 : Halfway\n"
            "           014.wav/mp3 : Tick\n\n"
            "  /02/  -> Spoken voice clips\n"
            "           001..020    : Yoga pose names\n"
            "           101..120    : Exercise move names\n"
            "           201..230    : Spoken numbers 1..30\n"
            "           251..260    : Guided cues ('Switch sides', 'Halfway', etc.)\n"
        )

    print()
    print("=" * 65)
    print(" Voice pack generation complete!")
    print(f" Ready to copy '{out_dir}/01' and '{out_dir}/02' to your DFPlayer microSD card.")
    print("=" * 65)


if __name__ == "__main__":
    asyncio.run(main())
