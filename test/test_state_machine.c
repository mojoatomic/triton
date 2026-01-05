/**
 * Unit Tests: Main State Machine
 *
 * Tests the main state machine transitions and outputs.
 * Run with: ./test/run_tests.sh test_state_machine
 */

#include "test_harness.h"
#include "mocks/mock_pico.h"

#include "control/state_machine.h"

TEST(state_machine_init_defaults) {
    StateMachine_t sm;

    state_machine_init(&sm);

    ASSERT_EQ((int)STATE_INIT, (int)state_machine_get_state(&sm));
    ASSERT_EQ(-100, state_machine_get_ballast_target(&sm));
    ASSERT_FALSE(state_machine_get_depth_hold_enabled(&sm));
}

TEST(state_machine_init_to_surface_transition) {
    StateMachine_t sm;

    state_machine_init(&sm);
    state_machine_process(&sm, CMD_NONE, 0, 100U);

    ASSERT_EQ((int)STATE_SURFACE, (int)state_machine_get_state(&sm));
    ASSERT_EQ(-100, state_machine_get_ballast_target(&sm));
    ASSERT_FALSE(state_machine_get_depth_hold_enabled(&sm));
}

TEST(state_machine_surface_dive_requires_target) {
    StateMachine_t sm;

    state_machine_init(&sm);
    state_machine_process(&sm, CMD_NONE, 0, 0U);

    state_machine_process(&sm, CMD_DIVE, 0, 10U);
    ASSERT_EQ((int)STATE_SURFACE, (int)state_machine_get_state(&sm));

    state_machine_set_target_depth(&sm, 100);
    state_machine_process(&sm, CMD_DIVE, 0, 20U);
    ASSERT_EQ((int)STATE_DIVING, (int)state_machine_get_state(&sm));
}

TEST(state_machine_diving_to_submerged_manual) {
    StateMachine_t sm;

    state_machine_init(&sm);
    state_machine_process(&sm, CMD_NONE, 0, 0U);
    state_machine_set_target_depth(&sm, 100);
    state_machine_process(&sm, CMD_DIVE, 0, 10U);

    ASSERT_EQ((int)STATE_DIVING, (int)state_machine_get_state(&sm));
    ASSERT_EQ(50, state_machine_get_ballast_target(&sm));

    state_machine_process(&sm, CMD_NONE, 60, 20U);
    ASSERT_EQ((int)STATE_SUBMERGED_MANUAL, (int)state_machine_get_state(&sm));
}

TEST(state_machine_depth_hold_enable_disable) {
    StateMachine_t sm;

    state_machine_init(&sm);
    state_machine_process(&sm, CMD_NONE, 0, 0U);
    state_machine_set_target_depth(&sm, 100);
    state_machine_process(&sm, CMD_DIVE, 0, 10U);
    state_machine_process(&sm, CMD_NONE, 60, 20U);

    ASSERT_EQ((int)STATE_SUBMERGED_MANUAL, (int)state_machine_get_state(&sm));

    state_machine_process(&sm, CMD_DEPTH_HOLD, 60, 30U);
    ASSERT_EQ((int)STATE_SUBMERGED_DEPTH_HOLD, (int)state_machine_get_state(&sm));
    ASSERT_TRUE(state_machine_get_depth_hold_enabled(&sm));

    state_machine_process(&sm, CMD_MANUAL, 60, 40U);
    ASSERT_EQ((int)STATE_SUBMERGED_MANUAL, (int)state_machine_get_state(&sm));
    ASSERT_FALSE(state_machine_get_depth_hold_enabled(&sm));
}

TEST(state_machine_emergency_terminal) {
    StateMachine_t sm;

    state_machine_init(&sm);
    state_machine_process(&sm, CMD_NONE, 0, 0U);

    state_machine_process(&sm, CMD_EMERGENCY, 0, 10U);
    ASSERT_EQ((int)STATE_EMERGENCY, (int)state_machine_get_state(&sm));

    state_machine_process(&sm, CMD_NONE, 0, 20U);
    ASSERT_EQ((int)STATE_EMERGENCY, (int)state_machine_get_state(&sm));
}

TEST(state_machine_surfacing_to_surface) {
    StateMachine_t sm;

    state_machine_init(&sm);
    state_machine_process(&sm, CMD_NONE, 0, 0U);
    state_machine_set_target_depth(&sm, 100);
    state_machine_process(&sm, CMD_DIVE, 0, 10U);

    state_machine_process(&sm, CMD_SURFACE, 100, 20U);
    ASSERT_EQ((int)STATE_SURFACING, (int)state_machine_get_state(&sm));
    ASSERT_EQ(-100, state_machine_get_ballast_target(&sm));

    state_machine_process(&sm, CMD_NONE, 5, 30U);
    ASSERT_EQ((int)STATE_SURFACE, (int)state_machine_get_state(&sm));
}

TEST_MAIN()
    RUN_TEST(state_machine_init_defaults);
    RUN_TEST(state_machine_init_to_surface_transition);
    RUN_TEST(state_machine_surface_dive_requires_target);
    RUN_TEST(state_machine_diving_to_submerged_manual);
    RUN_TEST(state_machine_depth_hold_enable_disable);
    RUN_TEST(state_machine_emergency_terminal);
    RUN_TEST(state_machine_surfacing_to_surface);
TEST_MAIN_END()
