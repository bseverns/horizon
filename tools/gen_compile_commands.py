#!/usr/bin/env python3
"""
Tiny helper to keep a host-friendly compile_commands.json around.

PlatformIO's `compiledb` target is the canonical source of truth, but CI or
offline editors don't always have PlatformIO installed. This script mirrors the
important flags from platformio.ini (stub includes + Teensy-ish defines) so
clangd can navigate the tree without a full toolchain download.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
OUTPUT = ROOT / "compile_commands.json"

common_args = [
    "clang++",
    "-std=gnu++14",
    "-DUSB_AUDIO",
    "-DARDUINO_TEENSY41",
    "-DHORIZON_BUILD_MAIN",
    "-I",
    "patches/cores/teensy4",
    "-I",
    "src",
    "-include",
    "stdint.h",
    "-include",
    "patches/cores/teensy4/lint_stubs.h",
]

commands = []
for cpp in sorted(SRC_DIR.glob("*.cpp")):
    commands.append(
        {
            "directory": str(ROOT),
            "file": str(cpp),
            "arguments": common_args + [str(cpp)],
        }
    )

OUTPUT.write_text(json.dumps(commands, indent=2))
print(f"Wrote {OUTPUT} with {len(commands)} translation units")
