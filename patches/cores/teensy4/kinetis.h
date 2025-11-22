#pragma once

// Compatibility shim: the Teensy Audio library still asks for "kinetis.h" in
// a few source files even when we target the IMXRT-based Teensy 4.x boards.
// Those builds really want the IMXRT register map, so we forward the include
// here. Think of this as gaffer tape keeping old include paths from tripping
// up the newer silicon.
#include "imxrt.h"
