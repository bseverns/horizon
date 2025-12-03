// Offline WAV driver for AudioHorizon.
//
// Loads a 44.1 kHz stereo WAV, feeds it into the Teensy-flavored
// AudioHorizon processor in 128-sample blocks, and writes the processed
// signal back to disk. The goal: make `pio test -e native_dsp` double as
// a tiny command-line render rig for the DSP core.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Horizon.h"

// -----------------------------------------------------------------------------
// Minimal WAV reader/writer (PCM16 only) so we don't drag in big deps. The
// format is constrained to what the Teensy Audio library expects: 44.1 kHz,
// stereo, 16-bit. That's enough for a quick offline bounce.
// -----------------------------------------------------------------------------

struct WavData {
    uint32_t sampleRate = 0;
    uint16_t channels = 0;
    std::vector<int16_t> samples; // interleaved
};

static bool read_le_u32(FILE *f, uint32_t &out) {
    uint8_t b[4];
    if (fread(b, 1, 4, f) != 4) return false;
    out = static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) |
          (static_cast<uint32_t>(b[2]) << 16) | (static_cast<uint32_t>(b[3]) << 24);
    return true;
}

static bool read_le_u16(FILE *f, uint16_t &out) {
    uint8_t b[2];
    if (fread(b, 1, 2, f) != 2) return false;
    out = static_cast<uint16_t>(b[0]) | (static_cast<uint16_t>(b[1]) << 8);
    return true;
}

static bool write_le_u32(FILE *f, uint32_t v) {
    uint8_t b[4] = {static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>((v >> 8) & 0xFF),
                   static_cast<uint8_t>((v >> 16) & 0xFF), static_cast<uint8_t>((v >> 24) & 0xFF)};
    return fwrite(b, 1, 4, f) == 4;
}

static bool write_le_u16(FILE *f, uint16_t v) {
    uint8_t b[2] = {static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>((v >> 8) & 0xFF)};
    return fwrite(b, 1, 2, f) == 2;
}

static bool read_wav(const std::string &path, WavData &out) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;

    char riff[4];
    if (fread(riff, 1, 4, f) != 4 || std::memcmp(riff, "RIFF", 4) != 0) {
        fclose(f);
        return false;
    }

    uint32_t chunkSize = 0;
    if (!read_le_u32(f, chunkSize)) {
        fclose(f);
        return false;
    }

    char wave[4];
    if (fread(wave, 1, 4, f) != 4 || std::memcmp(wave, "WAVE", 4) != 0) {
        fclose(f);
        return false;
    }

    bool foundFmt = false;
    bool foundData = false;
    uint16_t audioFormat = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataSize = 0;

    while (!foundFmt || !foundData) {
        char chunkId[4];
        if (fread(chunkId, 1, 4, f) != 4) break;
        uint32_t subChunkSize = 0;
        if (!read_le_u32(f, subChunkSize)) break;

        if (std::memcmp(chunkId, "fmt ", 4) == 0) {
            if (!read_le_u16(f, audioFormat)) break;
            if (!read_le_u16(f, out.channels)) break;
            if (!read_le_u32(f, out.sampleRate)) break;

            // skip byte rate + block align
            uint32_t throwaway32 = 0;
            uint16_t throwaway16 = 0;
            if (!read_le_u32(f, throwaway32)) break;
            if (!read_le_u16(f, throwaway16)) break;

            if (!read_le_u16(f, bitsPerSample)) break;

            if (subChunkSize > 16) {
                fseek(f, static_cast<long>(subChunkSize - 16), SEEK_CUR);
            }

            foundFmt = true;
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            dataSize = subChunkSize;
            std::vector<int16_t> samples(dataSize / sizeof(int16_t));
            size_t read = fread(samples.data(), 1, dataSize, f);
            samples.resize(read / sizeof(int16_t));
            out.samples = std::move(samples);
            foundData = true;
        } else {
            fseek(f, static_cast<long>(subChunkSize), SEEK_CUR);
        }

        if (ftell(f) >= static_cast<long>(chunkSize + 8)) {
            break; // Safety: avoid infinite loops on malformed files.
        }
    }

    fclose(f);

    return foundFmt && foundData && audioFormat == 1 && bitsPerSample == 16;
}

static bool write_wav(const std::string &path, const WavData &in) {
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return false;

    uint32_t dataSize = static_cast<uint32_t>(in.samples.size() * sizeof(int16_t));
    uint32_t fmtChunkSize = 16;
    uint32_t riffSize = 4 + (8 + fmtChunkSize) + (8 + dataSize);

    if (fwrite("RIFF", 1, 4, f) != 4 || !write_le_u32(f, riffSize) ||
        fwrite("WAVE", 1, 4, f) != 4) {
        fclose(f);
        return false;
    }

    // fmt chunk
    fwrite("fmt ", 1, 4, f);
    write_le_u32(f, fmtChunkSize);
    write_le_u16(f, 1); // PCM
    write_le_u16(f, in.channels);
    write_le_u32(f, in.sampleRate);
    uint16_t blockAlign = static_cast<uint16_t>(in.channels * sizeof(int16_t));
    uint32_t byteRate = in.sampleRate * blockAlign;
    write_le_u32(f, byteRate);
    write_le_u16(f, blockAlign);
    write_le_u16(f, 16); // bits per sample

    // data chunk
    fwrite("data", 1, 4, f);
    write_le_u32(f, dataSize);
    size_t written = fwrite(in.samples.data(), 1, dataSize, f);
    fclose(f);
    return written == dataSize;
}

// -----------------------------------------------------------------------------
// AudioStream hook and helper wrapper so we can queue blocks into AudioHorizon
// without a full Teensy runtime.
// -----------------------------------------------------------------------------

static std::vector<float> g_outL;
static std::vector<float> g_outR;

class HostHorizon : public AudioHorizon {
  public:
    void queueInput(audio_block_t *left, audio_block_t *right) {
        setInput(0, left);
        setInput(1, right);
    }
};

void horizon_host_transmit(audio_block_t *block, uint32_t channel, AudioStream *) {
    if (!block) return;
    std::vector<float> &dest = (channel == 0) ? g_outL : g_outR;
    dest.reserve(dest.size() + AUDIO_BLOCK_SAMPLES);
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
        dest.push_back(static_cast<float>(block->data[i]) / 32768.0f);
    }
}

// -----------------------------------------------------------------------------
int horizon_wav_driver(int argc, char **argv) {
    const std::string inPath = (argc > 1) ? argv[1] : "input.wav";
    const std::string outPath = (argc > 2) ? argv[2] : "output.wav";

    WavData input;
    if (!read_wav(inPath, input)) {
        std::fprintf(stderr, "[horizon] Failed to read WAV: %s\n", inPath.c_str());
        return 1;
    }

    if (input.channels != 2 || input.sampleRate != static_cast<uint32_t>(AUDIO_SAMPLE_RATE_EXACT)) {
        std::fprintf(stderr, "[horizon] Expected 44.1 kHz stereo WAV. Got %u Hz, %u channels.\n",
                     input.sampleRate, input.channels);
        return 1;
    }

    const size_t frames = input.samples.size() / 2;
    HostHorizon horizon;

    for (size_t frame = 0; frame < frames; frame += AUDIO_BLOCK_SAMPLES) {
        audio_block_t *inL = AudioStream::allocate();
        audio_block_t *inR = AudioStream::allocate();
        if (!inL || !inR) {
            std::fprintf(stderr, "[horizon] Allocation failed at frame %zu.\n", frame);
            return 1;
        }

        const size_t blockFrames = std::min(static_cast<size_t>(AUDIO_BLOCK_SAMPLES), frames - frame);
        for (size_t i = 0; i < blockFrames; ++i) {
            inL->data[i] = input.samples[2 * (frame + i) + 0];
            inR->data[i] = input.samples[2 * (frame + i) + 1];
        }
        for (size_t i = blockFrames; i < AUDIO_BLOCK_SAMPLES; ++i) {
            inL->data[i] = 0;
            inR->data[i] = 0;
        }

        horizon.queueInput(inL, inR);
        horizon.update();
    }

    const size_t producedFrames = std::min(g_outL.size(), g_outR.size());
    WavData output;
    output.channels = 2;
    output.sampleRate = static_cast<uint32_t>(AUDIO_SAMPLE_RATE_EXACT);
    output.samples.resize(producedFrames * 2);
    for (size_t i = 0; i < producedFrames; ++i) {
        const float l = std::max(-1.0f, std::min(1.0f, g_outL[i]));
        const float r = std::max(-1.0f, std::min(1.0f, g_outR[i]));
        output.samples[2 * i + 0] = static_cast<int16_t>(l * 32767.0f);
        output.samples[2 * i + 1] = static_cast<int16_t>(r * 32767.0f);
    }

    if (!write_wav(outPath, output)) {
        std::fprintf(stderr, "[horizon] Failed to write WAV: %s\n", outPath.c_str());
        return 1;
    }

    std::printf("[horizon] Rendered %zu frames to %s\n", producedFrames, outPath.c_str());
    return 0;
}

// -----------------------------------------------------------------------------
// Optional standalone entrypoint. Define HORIZON_WAV_STANDALONE to build a
// little command-line renderer instead of the Unity test runner. This keeps
// PlatformIO's `native_dsp` suite single-main while still letting curious folks
// bounce audio through the DSP from the shell.
// -----------------------------------------------------------------------------

#ifdef HORIZON_WAV_STANDALONE
int main(int argc, char **argv) { return horizon_wav_driver(argc, argv); }
#endif

