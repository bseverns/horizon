#pragma once

// Local shim around PJRC's dspinst helpers.
// GCC gets loud about missing returns in the inline asm wrappers; this overlay
// drops the "-Wreturn-type" shouting while still deferring to the real header.
// Think of it as gaffer tape on the console: not glamorous, but it keeps the
// session running clean for every Teensy build in this repo.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include_next "dspinst.h"
#pragma GCC diagnostic pop
