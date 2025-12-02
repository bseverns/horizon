#include <unity.h>
#include <cmath>

#include "SoftSaturation.h"
#include "ParamSmoother.h"
#include "DynWidth.h"
#include "LimiterLookahead.h"

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

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_soft_saturation_pass_through_when_dry);
    RUN_TEST(test_soft_saturation_stays_below_unity);
    RUN_TEST(test_param_smoother_seeds_and_glides);
    RUN_TEST(test_dynwidth_tracks_transient_activity);
    RUN_TEST(test_limiter_caps_hot_signal);
    RUN_TEST(test_led_mapping_matches_thresholds);
    return UNITY_END();
}
