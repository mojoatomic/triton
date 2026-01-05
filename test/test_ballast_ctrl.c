/**
 * Unit Tests: Ballast Controller
 *
 * Tests the ballast state machine.
 * Run with: ./test/run_tests.sh test_ballast_ctrl
 */

#include "test_harness.h"
#include "mocks/mock_pico.h"

#include "control/ballast_ctrl.h"

TEST(ballast_ctrl_init_defaults) {
    BallastController_t ctrl;

    ballast_ctrl_init(&ctrl);

    ASSERT_EQ((int)BALLAST_STATE_IDLE, (int)ballast_ctrl_get_state(&ctrl));
    ASSERT_EQ(0, ballast_ctrl_get_target(&ctrl));
    ASSERT_EQ(0, ballast_ctrl_get_current(&ctrl));
}

TEST(ballast_ctrl_set_target_clamps) {
    BallastController_t ctrl;
    ballast_ctrl_init(&ctrl);

    ballast_ctrl_set_target(&ctrl, 120);
    ASSERT_EQ(100, ballast_ctrl_get_target(&ctrl));

    ballast_ctrl_set_target(&ctrl, -120);
    ASSERT_EQ(-100, ballast_ctrl_get_target(&ctrl));
}

TEST(ballast_ctrl_update_from_idle_commands_fill) {
    BallastController_t ctrl;
    ballast_ctrl_init(&ctrl);
    ballast_ctrl_set_target(&ctrl, 50);

    int8_t pump = 0;
    bool valve_open = false;

    ballast_ctrl_update(&ctrl, 0U, &pump, &valve_open);

    ASSERT_EQ((int)BALLAST_STATE_FILLING, (int)ballast_ctrl_get_state(&ctrl));
    ASSERT_EQ(100, pump);
    ASSERT_FALSE(valve_open);
}

TEST(ballast_ctrl_update_advances_level_over_time) {
    BallastController_t ctrl;
    ballast_ctrl_init(&ctrl);
    ballast_ctrl_set_target(&ctrl, 100);

    int8_t pump = 0;
    bool valve_open = false;

    // Transition to filling
    ballast_ctrl_update(&ctrl, 0U, &pump, &valve_open);
    ASSERT_EQ(100, pump);

    // First update in filling sets the internal time base
    ballast_ctrl_update(&ctrl, 1000U, &pump, &valve_open);
    ASSERT_EQ(100, pump);

    // Second update should advance the estimate by ~20 (200 units / 10s = 20 per 1s)
    ballast_ctrl_update(&ctrl, 2000U, &pump, &valve_open);
    ASSERT_TRUE(ballast_ctrl_get_current(&ctrl) >= 15);
}

TEST(ballast_ctrl_reaches_target_and_holds) {
    BallastController_t ctrl;
    ballast_ctrl_init(&ctrl);
    ballast_ctrl_set_target(&ctrl, 10);

    int8_t pump = 0;
    bool valve_open = false;

    ballast_ctrl_update(&ctrl, 0U, &pump, &valve_open);
    ASSERT_EQ(100, pump);

    // Advance enough time to exceed target
    ballast_ctrl_update(&ctrl, 1000U, &pump, &valve_open);
    ballast_ctrl_update(&ctrl, 2000U, &pump, &valve_open);

    ASSERT_EQ((int)BALLAST_STATE_HOLDING, (int)ballast_ctrl_get_state(&ctrl));
    ASSERT_EQ(10, ballast_ctrl_get_current(&ctrl));

    // Holding state outputs should be safe
    ballast_ctrl_update(&ctrl, 3000U, &pump, &valve_open);
    ASSERT_EQ(0, pump);
    ASSERT_FALSE(valve_open);
}

TEST_MAIN()
    RUN_TEST(ballast_ctrl_init_defaults);
    RUN_TEST(ballast_ctrl_set_target_clamps);
    RUN_TEST(ballast_ctrl_update_from_idle_commands_fill);
    RUN_TEST(ballast_ctrl_update_advances_level_over_time);
    RUN_TEST(ballast_ctrl_reaches_target_and_holds);
TEST_MAIN_END()
