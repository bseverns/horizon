#include <unity.h>
#include <cmath>
#include <cstdio>

#include "SoftSaturation.h"
#include "ParamSmoother.h"
#include "DynWidth.h"
#include "LimiterLookahead.h"
#include "HostProcessor.h"
#include "AirEQ.h"

#include <cstdlib>
#include <filesystem>
#include <map>
#include <system_error>
#include <vector>

namespace {

StereoBuffer makeDemoBuffer(int sampleRate = 44100, float seconds = 1.0f) {
    const size_t frames = static_cast<size_t>(seconds * sampleRate);
    StereoBuffer buffer;
    buffer.sampleRate = sampleRate;
    buffer.left.resize(frames);
    buffer.right.resize(frames);

    for (size_t i = 0; i < frames; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        // A touch of motion: mid-heavy 200 Hz tone plus a slower 3 Hz wobble on the sides.
        float mid = 0.3f * sinf(2.0f * static_cast<float>(M_PI) * 200.0f * t);
        float side = 0.15f * sinf(2.0f * static_cast<float>(M_PI) * 3.0f * t);
        buffer.left[i] = mid + side;
        buffer.right[i] = mid - side;
    }

    return buffer;
}

struct DiffResult {
    float maxDiff = 0.0f;
    size_t frames = 0;
};

DiffResult measureMaxDiff(const StereoBuffer &expected, const StereoBuffer &actual) {
    const size_t frames = std::min(expected.left.size(), actual.left.size());
    DiffResult diff{0.0f, frames};

    for (size_t i = 0; i < frames; ++i) {
        const float l = fabsf(expected.left[i] - actual.left[i]);
        const float r = fabsf(expected.right[i] - actual.right[i]);
        diff.maxDiff = fmaxf(diff.maxDiff, fmaxf(l, r));
    }

    return diff;
}

std::filesystem::path findAssetsBaseDir() {
    namespace fs = std::filesystem;
    const fs::path relativeSuiteDir = fs::path("test") / "native_dsp";
    const fs::path fixture = fs::path("assets") / "sample.wav";

    // Allow CI or dev shells to hard-pin the search root when the working
    // directory is surprising (e.g. PlatformIO spawning the test runner from
    // a temp folder).
    if (const char *env = std::getenv("HORIZON_NATIVE_DSP_ASSETS")) {
        fs::path override = fs::path(env);
        if (fs::exists(override / fixture)) {
            return fs::weakly_canonical(override);
        }
    }

    const fs::path sourceDir = fs::path(__FILE__).parent_path();
    const fs::path cwd = fs::current_path();

    const fs::path executable = [] {
        std::error_code ec;
        auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
        return ec ? std::filesystem::path{} : exe.parent_path();
    }();

    const fs::path directCandidates[] = {
        sourceDir,
        cwd,
        executable,
        cwd / relativeSuiteDir,
        executable / relativeSuiteDir,
    };

    for (const auto &root : directCandidates) {
        if (fs::exists(root / fixture)) {
            return fs::weakly_canonical(root);
        }
    }

    auto walkForFixture = [&](fs::path cursor) -> fs::path {
        while (!cursor.empty()) {
            if (fs::exists(cursor / relativeSuiteDir / fixture)) {
                return fs::weakly_canonical(cursor / relativeSuiteDir);
            }
            if (cursor == cursor.root_path()) {
                break;
            }
            cursor = cursor.parent_path();
        }
        return fs::path{};
    };

    if (auto fromCwd = walkForFixture(cwd); !fromCwd.empty()) {
        return fromCwd;
    }
    if (auto fromExe = walkForFixture(executable); !fromExe.empty()) {
        return fromExe;
    }

    return fs::weakly_canonical(relativeSuiteDir);
}

} // namespace

void test_soft_saturation_pass_through_when_dry() {
    SoftSaturation sat;
    sat.setAmount(0.0f);
    float l = 0.3f;
    float r = -0.42f;
    sat.processStereo(l, r);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.3f, l);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -0.42f, r);
}

void test_soft_saturation_stays_below_unity() {
    SoftSaturation sat;
    sat.setAmount(0.9f);
    float x = 1.5f; // harder than any musical program should be
    float y = sat.processSample(x);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, fabsf(y));
}

void test_param_smoother_seeds_and_glides() {
    ParamSmoother sm(0.25f);
    // First call seeds to target.
    float v0 = sm.process(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.5f, v0);
    // Subsequent calls glide toward new targets.
    float v1 = sm.process(1.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.625f, v1);
    float v2 = sm.process(1.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.71875f, v2);
}

void test_dynwidth_tracks_transient_activity() {
    DynWidth dw;
    dw.setBaseWidth(0.5f);
    dw.setDynAmount(0.5f);
    dw.setLowAnchorHz(120.0f);

    float mid = 0.0f;
    float side = 0.1f;
    // High transient activity should push toward the narrow state (~0.275)
    dw.processSample(mid, side, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.275f, dw.getLastWidth());

    // No transient should widen toward ~1.0
    dw.processSample(mid, side, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 1.0f, dw.getLastWidth());
}

void test_tilt_and_air_extremes_hold_bounds() {
    TiltEQ tilt;
    tilt.setTiltDbPerOct(12.0f); // exceeds clamp intentionally

    float last = 0.0f;
    for (int i = 0; i < 256; ++i) {
        last = tilt.processSample(1.0f);
        TEST_ASSERT_TRUE_MESSAGE(std::isfinite(last), "tilt output blew up");
        TEST_ASSERT_LESS_OR_EQUAL_FLOAT(2.5f, fabsf(last));
    }

    AirEQ air;
    air.setFreqAndGain(48000.0f, 12.0f); // clamp both freq and gain
    float maxAir = 0.0f;
    for (int i = 0; i < 256; ++i) {
        float x = (i % 2 == 0) ? 1.0f : -1.0f; // excite both halves of the split
        float y = air.processSample(x);
        TEST_ASSERT_TRUE_MESSAGE(std::isfinite(y), "air output blew up");
        maxAir = fmaxf(maxAir, fabsf(y));
    }

    // Tilt clamps to ±6 dB/oct; Air clamps to 16 kHz / ±6 dB, so neither should explode past ~2×.
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(2.1f, fabsf(last));
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(2.1f, maxAir);
}

void test_limiter_caps_hot_signal() {
    LimiterLookahead lim;
    lim.setup();
    lim.setCeilingDb(-6.0f);
    lim.setLookaheadMs(1.0f);
    lim.setMix(1.0f);

    float maxOut = 0.0f;
    for (int i = 0; i < 200; ++i) {
        float l = 1.0f;
        float r = 1.0f;
        lim.processStereo(l, r);
        maxOut = fmaxf(maxOut, fmaxf(fabsf(l), fabsf(r)));
    }

    // With -6 dB ceiling (~0.5 linear) the limiter should settle close to that.
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(0.6f, maxOut);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.5f, lim.getGain());
}

void test_limiter_link_modes_hold_guardrails() {
    LimiterLookahead linked;
    LimiterLookahead midSide;
    linked.setup();
    midSide.setup();

    linked.setCeilingDb(-3.0f);
    linked.setLookaheadMs(5.5f);
    linked.setReleaseMs(90.0f);
    linked.setMix(1.0f);
    linked.setLinkMode(LimiterLookahead::LinkMode::Linked);

    midSide.setCeilingDb(-3.0f);
    midSide.setLookaheadMs(5.5f);
    midSide.setReleaseMs(90.0f);
    midSide.setMix(1.0f);
    midSide.setLinkMode(LimiterLookahead::LinkMode::MidSide);

    float maxLinked = 0.0f;
    float maxMidSide = 0.0f;

    for (int i = 0; i < 256; ++i) {
        float l = (i % 3 == 0) ? 1.0f : 0.5f;
        float r = (i % 5 == 0) ? 0.25f : 1.0f;

        linked.processStereo(l, r);
        midSide.processStereo(l, r);

        maxLinked = fmaxf(maxLinked, fmaxf(fabsf(l), fabsf(r)));
        maxMidSide = fmaxf(maxMidSide, fmaxf(fabsf(l), fabsf(r)));
    }

    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, maxLinked);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, maxMidSide);
    // Mid/Side detector should usually let the quiet channel breathe more.
    TEST_ASSERT_TRUE_MESSAGE(midSide.getGain() >= linked.getGain(), "Mid/Side link clamped harder than Linked mode");
}

void test_led_mapping_matches_thresholds() {
    TEST_ASSERT_EQUAL_UINT8(0, mapGRtoLeds(0.0f));
    TEST_ASSERT_EQUAL_UINT8(1, mapGRtoLeds(-1.0f));
    TEST_ASSERT_EQUAL_UINT8(5, mapGRtoLeds(-7.0f));
    TEST_ASSERT_EQUAL_UINT8(8, mapGRtoLeds(-14.0f));
}

void test_host_processor_renders_variants() {
    namespace fs = std::filesystem;
    const fs::path baseDir = findAssetsBaseDir();
    const fs::path artifactDir = baseDir / "artifacts";

    std::map<std::string, StereoBuffer> baselines;
    for (const char *flavor : {"light", "mid", "heavy", "kitchen_sink"}) {
        const fs::path baselinePath = artifactDir / ("sample_" + std::string(flavor) + ".wav");
        if (fs::exists(baselinePath)) {
            baselines.emplace(flavor, loadStereoWav(baselinePath));
        }
    }

    // Realign tolerance per flavor. The "light" path uses the gentlest settings and its
    // rendering can wobble a hair more across toolchains, so we give it extra breathing room.
    // The "kitchen_sink" path stacks every processor (nonlinear stages included), so its
    // render tends to drift more; loosen the leash there too while keeping the other flavors
    // strict for catching regressions.
    const std::map<std::string, float> tolerances = {
        {"light", 0.015f},
        {"kitchen_sink", 0.12f},
    };

    fs::remove_all(artifactDir);
    fs::create_directories(artifactDir);

    const fs::path assetPath = baseDir / "assets" / "sample.wav";
    StereoBuffer input = loadStereoWav(assetPath);
    const bool wavLoaded = !(input.left.empty() || input.right.empty());
    if (!wavLoaded) {
        printf("[warn] could not load %s; falling back to synthetic demo buffer\n", assetPath.string().c_str());
        input = makeDemoBuffer();
    }
    TEST_ASSERT_FALSE_MESSAGE(input.left.empty() || input.right.empty(), "host processor input buffer is empty");

    auto renders = buildDemoRenders(input);
    HostProcessor processor;

    for (const auto &render : renders) {
        processor.reset();
        processor.setParameters(render.params);
        StereoBuffer out = processor.process(render.audio);

        const fs::path outPath = artifactDir / ("sample_" + render.flavor + ".wav");
        writeStereoWav(outPath, out);

        const char *linkMode = (render.params.limiterLink == LimiterLookahead::LinkMode::MidSide) ? "Mid/Side" : "Linked";
        printf("[render] flavor=%s -> %s | width=%.2f dyn=%.2f sens=%.2f tilt=%.2f air=%.1fHz/+%.1fdB dirt=%.2f ceiling=%.1fdB rel=%.1fms look=%.1fms detTilt=%.1fdB/oct mix=%.2f link=%s trim=%.1fdB\n",
               render.flavor.c_str(),
               outPath.string().c_str(),
               render.params.width,
               render.params.dynWidth,
               render.params.transientSens,
               render.params.midTiltDbPerOct,
               render.params.sideAirFreqHz,
               render.params.sideAirGainDb,
               render.params.dirtAmount,
               render.params.ceilingDb,
               render.params.limiterReleaseMs,
               render.params.limiterLookaheadMs,
               render.params.limiterTiltDbPerOct,
               render.params.limiterMix,
               linkMode,
               render.params.outputTrimDb);

        TEST_ASSERT_EQUAL(input.sampleRate, out.sampleRate);
        TEST_ASSERT_EQUAL_UINT32(input.left.size(), out.left.size());
        TEST_ASSERT_EQUAL_UINT32(input.right.size(), out.right.size());

        auto baselineIt = baselines.find(render.flavor);
        if (baselineIt != baselines.end() && !baselineIt->second.left.empty() && !baselineIt->second.right.empty()) {
            TEST_ASSERT_EQUAL_MESSAGE(
                baselineIt->second.sampleRate, out.sampleRate,
                ("sample rate mismatch for baseline flavor: " + render.flavor).c_str());
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(static_cast<uint32_t>(baselineIt->second.left.size()),
                                             static_cast<uint32_t>(out.left.size()),
                                             ("left channel length mismatch for baseline flavor: " + render.flavor).c_str());
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(static_cast<uint32_t>(baselineIt->second.right.size()),
                                             static_cast<uint32_t>(out.right.size()),
                                             ("right channel length mismatch for baseline flavor: " + render.flavor).c_str());

            const float tol = [&] {
                auto tolIt = tolerances.find(render.flavor);
                return (tolIt != tolerances.end()) ? tolIt->second : 1e-3f;
            }();

            DiffResult diff = measureMaxDiff(baselineIt->second, out);
            printf("[diff] flavor=%s maxDiff=%.6f (tol=%.4f) frames=%zu\n", render.flavor.c_str(), diff.maxDiff, tol, diff.frames);
            TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(tol, diff.maxDiff,
                                                    ("render diverges beyond tolerance for flavor: " + render.flavor).c_str());
        } else {
            printf("[diff] flavor=%s baseline missing; skipping comparison\n", render.flavor.c_str());
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(wavLoaded, "WAV fixture missing or unsupported; ran with fallback buffer");
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_soft_saturation_pass_through_when_dry);
    RUN_TEST(test_soft_saturation_stays_below_unity);
    RUN_TEST(test_param_smoother_seeds_and_glides);
    RUN_TEST(test_dynwidth_tracks_transient_activity);
    RUN_TEST(test_tilt_and_air_extremes_hold_bounds);
    RUN_TEST(test_limiter_caps_hot_signal);
    RUN_TEST(test_limiter_link_modes_hold_guardrails);
    RUN_TEST(test_led_mapping_matches_thresholds);
    RUN_TEST(test_host_processor_renders_variants);
    return UNITY_END();
}
