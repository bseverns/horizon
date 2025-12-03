#pragma once
// Minimal PJRC Audio header shim for host builds. We only surface the constants
// Horizon's limiter expects; anything more should be added with care so we keep
// dependencies light for plugin builds.

constexpr float AUDIO_SAMPLE_RATE_EXACT = 44100.0f;
constexpr int AUDIO_BLOCK_SAMPLES = 128;
