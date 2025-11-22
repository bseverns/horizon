"""
Opt-in lint-only shim wiring.

We keep the host-only stubs out of normal firmware builds so Teensy picks up
F_CPU_ACTUAL, NVIC helpers, and friends directly from PJRC's core headers.
Set HORIZON_LINT_STUBS=1 (or use a PlatformIO env whose name starts with
"lint"/"clangd") when generating editor metadata so clangd has something to
chew on without a full toolchain install.
"""
import os
from SCons.Script import Import

Import("env")

pioenv = env.subst("$PIOENV")
use_lint_stubs = (
    os.environ.get("HORIZON_LINT_STUBS") == "1"
    or pioenv.startswith("lint")
    or pioenv.startswith("clangd")
)

if use_lint_stubs:
    env.AppendUnique(CCFLAGS=["-include", "patches/cores/teensy4/lint_stubs.h"])
    env.AppendUnique(CXXFLAGS=["-include", "patches/cores/teensy4/lint_stubs.h"])
