#include <unity.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "SoftSaturation.h"
#include "ParamSmoother.h"
#include "DynWidth.h"
#include "LimiterLookahead.h"
#include "HostProcessor.h"
#include "host/HostHorizonProcessor.h"

#include <cstdlib>
#include <filesystem>
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

float maxAbsDiff(const StereoBuffer &a, const StereoBuffer &b) {
    const size_t total = std::min(std::min(a.left.size(), b.left.size()), std::min(a.right.size(), b.right.size()));
    float maxDiff = 0.0f;
    for (size_t i = 0; i < total; ++i) {
        maxDiff = fmaxf(maxDiff, fabsf(a.left[i] - b.left[i]));
        maxDiff = fmaxf(maxDiff, fabsf(a.right[i] - b.right[i]));
    }
    return maxDiff;
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
    fs::remove_all(artifactDir);
    fs::create_directories(artifactDir);

    const fs::path assetPath = baseDir / "assets" / "sample.wav";
    const fs::path baselineLight = baseDir / "sample_light.wav";

    StereoBuffer input = loadStereoWav(assetPath);
    StereoBuffer baselineProbe = loadStereoWav(baselineLight);

    // Keep the render length aligned with the checked-in baselines so small fixture WAVs don’t
    // balloon the comparison. Fallback: if the baseline is missing, keep the full input length.
    const size_t targetFrames = baselineProbe.left.empty() ? input.left.size() : baselineProbe.left.size();
    const int targetSampleRate = baselineProbe.left.empty() ? input.sampleRate : baselineProbe.sampleRate;
    if (input.left.size() > targetFrames) {
        input.left.resize(targetFrames);
        input.right.resize(targetFrames);
    }
    input.sampleRate = targetSampleRate;

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

        const fs::path baselinePath = baseDir / ("sample_" + render.flavor + ".wav");
        StereoBuffer baseline = loadStereoWav(baselinePath);
        TEST_ASSERT_FALSE_MESSAGE(baseline.left.empty() || baseline.right.empty(),
                                  ("missing baseline render: " + baselinePath.string()).c_str());
        TEST_ASSERT_EQUAL_MESSAGE(baseline.sampleRate, out.sampleRate, "baseline sample rate mismatch");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(baseline.left.size(), out.left.size(), "baseline left length mismatch");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(baseline.right.size(), out.right.size(), "baseline right length mismatch");

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

        float maxDiff = maxAbsDiff(out, baseline);
        TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(1e-3f, maxDiff, "render diverges from baseline WAV");
    }

    TEST_ASSERT_TRUE_MESSAGE(wavLoaded, "WAV fixture missing or unsupported; ran with fallback buffer");
}

void test_host_horizon_processor_smoke() {
    HostHorizonProcessor pluginProcessor;
    pluginProcessor.prepareToPlay(48000.0, 256);
    pluginProcessor.setWidth(0.8f);
    pluginProcessor.setDynWidth(0.2f);
    pluginProcessor.setTransientSens(0.6f);
    pluginProcessor.setMidTilt(0.5f);
    pluginProcessor.setSideAir(12000.0f, 3.0f);
    pluginProcessor.setLowAnchor(130.0f);
    pluginProcessor.setDirt(0.25f);
    pluginProcessor.setCeiling(-3.0f);
    pluginProcessor.setLimiterReleaseMs(90.0f);
    pluginProcessor.setLimiterLookaheadMs(6.5f);
    pluginProcessor.setLimiterDetectorTilt(1.5f);
    pluginProcessor.setLimiterLinkMode(LimiterLookahead::LinkMode::MidSide);
    pluginProcessor.setLimiterMix(0.9f);
    pluginProcessor.setLimiterBypass(false);
    pluginProcessor.setMix(1.0f);
    pluginProcessor.setOutputTrim(-1.0f);

    constexpr int kFrames = 256;
    std::vector<float> silence(kFrames, 0.0f);
    std::vector<float> impulse(kFrames, 0.0f);
    impulse[0] = 1.0f;

    std::vector<float> outL(kFrames, 0.0f);
    std::vector<float> outR(kFrames, 0.0f);

    pluginProcessor.processBlock(silence.data(), silence.data(), outL.data(), outR.data(), kFrames, 48000.0);
    for (int i = 0; i < kFrames; ++i) {
        TEST_ASSERT_TRUE_MESSAGE(std::isfinite(outL[i]) && std::isfinite(outR[i]), "silence render produced non-finite samples");
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, outL[i]);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, outR[i]);
    }

    std::fill(outL.begin(), outL.end(), 0.0f);
    std::fill(outR.begin(), outR.end(), 0.0f);

    pluginProcessor.processBlock(impulse.data(), impulse.data(), outL.data(), outR.data(), kFrames, 48000.0);
    pluginProcessor.processBlock(silence.data(), silence.data(), outL.data(), outR.data(), kFrames, 48000.0);
    bool anyOutput = false;
    for (int i = 0; i < kFrames; ++i) {
        TEST_ASSERT_TRUE_MESSAGE(std::isfinite(outL[i]) && std::isfinite(outR[i]), "impulse render produced non-finite samples");
        anyOutput = anyOutput || fabsf(outL[i]) > 1e-5f || fabsf(outR[i]) > 1e-5f;
    }
    TEST_ASSERT_TRUE_MESSAGE(anyOutput, "impulse render was entirely silent after lookahead latency");
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_soft_saturation_pass_through_when_dry);
    RUN_TEST(test_soft_saturation_stays_below_unity);
    RUN_TEST(test_param_smoother_seeds_and_glides);
    RUN_TEST(test_dynwidth_tracks_transient_activity);
    RUN_TEST(test_limiter_caps_hot_signal);
    RUN_TEST(test_led_mapping_matches_thresholds);
    RUN_TEST(test_host_processor_renders_variants);
    RUN_TEST(test_host_horizon_processor_smoke);
    return UNITY_END();
}
