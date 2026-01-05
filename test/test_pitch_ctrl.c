/**
 * Unit Tests: Pitch Controller
 *
 * Tests the pitch controller wrapper around PID.
 * Run with: ./test/run_tests.sh test_pitch_ctrl
 */

#include "test_harness.h"
#include "mocks/mock_pico.h"

#include "control/pitch_ctrl.h"

TEST(pitch_ctrl_init_defaults) {
    PitchController_t ctrl;

    pitch_ctrl_init(&ctrl);

    ASSERT_EQ(0, ctrl.target_pitch_x10);
    ASSERT_TRUE(ctrl.enabled);
}

TEST(pitch_ctrl_update_disabled_zero) {
    PitchController_t ctrl;

    pitch_ctrl_init(&ctrl);
    pitch_ctrl_enable(&ctrl, false);

    const int8_t out = pitch_ctrl_update(&ctrl, 0, 0.1f);
    ASSERT_EQ(0, out);
}

TEST(pitch_ctrl_enable_resets_pid) {
    PitchController_t ctrl;

    pitch_ctrl_init(&ctrl);

    ctrl.pid.integral = 123.0f;
    ctrl.pid.prev_error = 456.0f;
    ctrl.pid.prev_measurement = 789.0f;

    pitch_ctrl_enable(&ctrl, false);
    pitch_ctrl_enable(&ctrl, true);

    ASSERT_FLOAT_EQ(ctrl.pid.integral, 0.0f, 0.0001f);
    ASSERT_FLOAT_EQ(ctrl.pid.prev_error, 0.0f, 0.0001f);
    ASSERT_FLOAT_EQ(ctrl.pid.prev_measurement, 0.0f, 0.0001f);
    ASSERT_TRUE(ctrl.enabled);
}

TEST(pitch_ctrl_set_target_out_of_range_no_change) {
    PitchController_t ctrl;

    pitch_ctrl_init(&ctrl);

    pitch_ctrl_set_target(&ctrl, 100);
    ASSERT_EQ(100, ctrl.target_pitch_x10);

    // MAX_PITCH_DEG is 45, so limit is +/-450 (x10 units)
    pitch_ctrl_set_target(&ctrl, 1000);
    ASSERT_EQ(100, ctrl.target_pitch_x10);

    pitch_ctrl_set_target(&ctrl, -1000);
    ASSERT_EQ(100, ctrl.target_pitch_x10);
}

TEST(pitch_ctrl_update_sign) {
    PitchController_t ctrl;

    pitch_ctrl_init(&ctrl);
    pitch_ctrl_set_target(&ctrl, 0);

    const int8_t out_nose_down = pitch_ctrl_update(&ctrl, -100, 0.1f);
    ASSERT_TRUE(out_nose_down > 0);

    const int8_t out_nose_up = pitch_ctrl_update(&ctrl, 100, 0.1f);
    ASSERT_TRUE(out_nose_up < 0);
}

TEST_MAIN()
    RUN_TEST(pitch_ctrl_init_defaults);
    RUN_TEST(pitch_ctrl_update_disabled_zero);
    RUN_TEST(pitch_ctrl_enable_resets_pid);
    RUN_TEST(pitch_ctrl_set_target_out_of_range_no_change);
    RUN_TEST(pitch_ctrl_update_sign);
TEST_MAIN_END()
