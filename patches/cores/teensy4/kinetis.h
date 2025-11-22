// Stub kinetis.h for Teensy 4.x builds.
// The PJRC Audio library's legacy ADC path pulls in utility/pdb.h, which hard-requires
// kinetis.h even though that hardware block only exists on the Kinetis-based Teensy 3.x.
// On IMXRT (Teensy 4.x) we just need the include to succeed so the translation unit can
// compile down to nothing when KINETISK isn't defined.
//
// This header intentionally leaves KINETISK undefined and exports no symbols. It's a
// compatibility shim, not a hardware definition.
#ifndef HORIZON_STUB_KINETIS_H
#define HORIZON_STUB_KINETIS_H

#endif // HORIZON_STUB_KINETIS_H
