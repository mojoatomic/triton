/**
 * Unit Tests: Depth Controller
 *
 * Tests the depth controller wrapper around PID.
 * Run with: ./test/run_tests.sh test_depth_ctrl
 */

#include "test_harness.h"
#include "mocks/mock_pico.h"

#include "control/depth_ctrl.h"

TEST(depth_ctrl_init_defaults) {
    DepthController_t ctrl;

    depth_ctrl_init(&ctrl);

    ASSERT_EQ(0, ctrl.target_depth_cm);
    ASSERT_FALSE(ctrl.enabled);
}

TEST(depth_ctrl_update_disabled_zero) {
    DepthController_t ctrl;

    depth_ctrl_init(&ctrl);
    depth_ctrl_set_target(&ctrl, 100);

    int8_t out = depth_ctrl_update(&ctrl, 0, 0.1f);
    ASSERT_EQ(0, out);
}

TEST(depth_ctrl_enable_resets_pid) {
    DepthController_t ctrl;

    depth_ctrl_init(&ctrl);

    ctrl.pid.integral = 123.0f;
    ctrl.pid.prev_error = 456.0f;
    ctrl.pid.prev_measurement = 789.0f;

    depth_ctrl_enable(&ctrl, true);

    ASSERT_FLOAT_EQ(ctrl.pid.integral, 0.0f, 0.0001f);
    ASSERT_FLOAT_EQ(ctrl.pid.prev_error, 0.0f, 0.0001f);
    ASSERT_FLOAT_EQ(ctrl.pid.prev_measurement, 0.0f, 0.0001f);
    ASSERT_TRUE(ctrl.enabled);
}

TEST(depth_ctrl_update_sign) {
    DepthController_t ctrl;

    depth_ctrl_init(&ctrl);
    depth_ctrl_enable(&ctrl, true);

    depth_ctrl_set_target(&ctrl, 100);
    int8_t out_down = depth_ctrl_update(&ctrl, 0, 0.1f);
    ASSERT_TRUE(out_down > 0);

    depth_ctrl_set_target(&ctrl, 0);
    int8_t out_up = depth_ctrl_update(&ctrl, 100, 0.1f);
    ASSERT_TRUE(out_up < 0);
}

TEST_MAIN()
    RUN_TEST(depth_ctrl_init_defaults);
    RUN_TEST(depth_ctrl_update_disabled_zero);
    RUN_TEST(depth_ctrl_enable_resets_pid);
    RUN_TEST(depth_ctrl_update_sign);
TEST_MAIN_END()
