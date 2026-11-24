#!/usr/bin/env python3
"""
Assemble the Coriolis standalone website (_site/) and compile the C++11
WebAssembly emulator from web/main.cpp + shared core/ and scenes/.

Usage:
  python tools/build_site.py --build-wasm --out _site
"""

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

EXPORTED_FUNCTIONS = [
    "_coriolis_init",
    "_coriolis_set_time",
    "_coriolis_tick",
    "_coriolis_rgba_ptr",
    "_coriolis_key",
    "_coriolis_cmd",
    "_coriolis_set_held",
    "_coriolis_show",
    "_coriolis_rotate",
    "_coriolis_toggle_night",
    "_coriolis_is_night",
    "_coriolis_night_enabled",
    "_coriolis_cycle_weather",
    "_coriolis_scene_count",
    "_coriolis_scene_name",
    "_coriolis_active_name",
    "_coriolis_palette_name",
]


def emcc_args():
    return [
        "em++",
        "-std=c++11",
        "-O2",
        "-Wall",
        "-Wextra",
        "web/main.cpp",
        "-o",
        "web/build/coriolis.js",
        "-sMODULARIZE=1",
        "-sEXPORT_NAME=CoriolisModule",
        "-sENVIRONMENT=web",
        "-sALLOW_MEMORY_GROWTH=0",
        "-sINITIAL_MEMORY=16777216",
        f"-sEXPORTED_FUNCTIONS={json.dumps(EXPORTED_FUNCTIONS, separators=(',', ':'))}",
        "-sEXPORTED_RUNTIME_METHODS=UTF8ToString,HEAPU8",
    ]


def build_wasm():
    out_dir = ROOT / "web" / "build"
    out_dir.mkdir(parents=True, exist_ok=True)
    args = emcc_args()

    emcc = shutil.which("em++")
    if emcc:
        cmd = [emcc] + args[1:]
        print("Running local Emscripten:", " ".join(cmd))
        subprocess.run(cmd, cwd=ROOT, check=True)
        return

    docker = shutil.which("docker")
    if docker:
        mount = f"{ROOT.as_posix()}:/src"
        # Inside Docker (Linux shell-free exec), comma-separated or JSON both
        # work cleanly since subprocess.run passes argv directly without shell.
        cmd = [
            docker,
            "run",
            "--rm",
            "-v",
            mount,
            "-w",
            "/src",
            "emscripten/emsdk:latest",
        ] + args
        print("Running Docker Emscripten:", " ".join(cmd))
        subprocess.run(cmd, cwd=ROOT, check=True)
        return

    sys.exit(
        "Neither em++ nor docker was found on PATH. Install Emscripten or "
        "Docker to build web/build/coriolis.wasm."
    )


def copy_tree_contents(src_dir, dst_dir):
    for item in src_dir.iterdir():
        target = dst_dir / item.name
        if item.is_dir():
            shutil.copytree(item, target, dirs_exist_ok=True)
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(item, target)


def assemble_site(out_dir, force_wasm):
    wasm_js = ROOT / "web" / "build" / "coriolis.js"
    wasm_bin = ROOT / "web" / "build" / "coriolis.wasm"
    if force_wasm or not wasm_js.exists() or not wasm_bin.exists():
        build_wasm()

    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # 1. Static showcase files
    copy_tree_contents(ROOT / "site", out_dir)

    # 2. Emscripten WASM bundle
    emu_out = out_dir / "emu"
    emu_out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(wasm_js, emu_out / "coriolis.js")
    shutil.copy2(wasm_bin, emu_out / "coriolis.wasm")

    # 3. Regenerated screenshots
    shots_out = out_dir / "img" / "screenshots"
    shots_out.mkdir(parents=True, exist_ok=True)
    for png in sorted((ROOT / "docs" / "screenshots").glob("*.png")):
        shutil.copy2(png, shots_out / png.name)

    # 4. Printable PDFs
    manuals_out = out_dir / "manuals"
    manuals_out.mkdir(parents=True, exist_ok=True)
    for pdf_name in ("Coriolis_User_Manual.pdf", "Coriolis_Build_Guide.pdf"):
        src_pdf = ROOT / "docs" / pdf_name
        if not src_pdf.exists():
            sys.exit(f"Missing required manual PDF: {src_pdf}")
        shutil.copy2(src_pdf, manuals_out / pdf_name)

    print(f"Assembled site in {out_dir}")


def main():
    parser = argparse.ArgumentParser(description="Build Coriolis website + WASM")
    parser.add_argument(
        "--build-wasm",
        action="store_true",
        help="Recompile web/main.cpp to WebAssembly before assembling _site/",
    )
    parser.add_argument(
        "--out",
        default="_site",
        help="Output directory (default: _site)",
    )
    args = parser.parse_args()
    out_path = Path(args.out)
    if not out_path.is_absolute():
        out_path = ROOT / out_path
    assemble_site(out_path, args.build_wasm)


if __name__ == "__main__":
    main()
