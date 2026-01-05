/**
 * Unit Tests: PID Controller
 *
 * Tests the generic PID controller module.
 * Run with: ./test/run_tests.sh test_pid
 */

#include "test_harness.h"
#include "mocks/mock_pico.h"

// Include the module under test
#include "control/pid.h"

// ============================================================
// Tests
// ============================================================

TEST(pid_init_sets_gains) {
    PidController_t pid;
    pid_init(&pid, 1.0f, 0.5f, 0.25f);

    ASSERT_FLOAT_EQ(pid.kp, 1.0f, 0.001f);
    ASSERT_FLOAT_EQ(pid.ki, 0.5f, 0.001f);
    ASSERT_FLOAT_EQ(pid.kd, 0.25f, 0.001f);
}

TEST(pid_init_clears_state) {
    PidController_t pid;
    pid.integral = 999.0f;  // Set garbage values
    pid.prev_error = 999.0f;

    pid_init(&pid, 1.0f, 0.0f, 0.0f);

    ASSERT_FLOAT_EQ(pid.integral, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(pid.prev_error, 0.0f, 0.001f);
}

TEST(pid_proportional_only) {
    PidController_t pid;
    pid_init(&pid, 2.0f, 0.0f, 0.0f);  // Kp=2, Ki=0, Kd=0

    // Setpoint=100, measurement=0, error=100
    // P output = 2.0 * 100 = 200, but clamped to 100
    float output = pid_update(&pid, 100.0f, 0.0f, 0.02f);
    ASSERT_FLOAT_EQ(output, 100.0f, 0.001f);  // Clamped

    // Smaller error: setpoint=50, measurement=40, error=10
    pid_reset(&pid);
    output = pid_update(&pid, 50.0f, 40.0f, 0.02f);
    ASSERT_FLOAT_EQ(output, 20.0f, 0.001f);  // 2.0 * 10 = 20
}

TEST(pid_integral_accumulates) {
    PidController_t pid;
    pid_init(&pid, 0.0f, 1.0f, 0.0f);  // Ki=1 only

    float dt = 0.1f;

    // First update: error=10, integral = 10 * 0.1 = 1
    float output = pid_update(&pid, 10.0f, 0.0f, dt);
    ASSERT_FLOAT_EQ(output, 1.0f, 0.001f);

    // Second update: error still 10, integral = 1 + 1 = 2
    output = pid_update(&pid, 10.0f, 0.0f, dt);
    ASSERT_FLOAT_EQ(output, 2.0f, 0.001f);

    // Third update: integral = 3
    output = pid_update(&pid, 10.0f, 0.0f, dt);
    ASSERT_FLOAT_EQ(output, 3.0f, 0.001f);
}

TEST(pid_integral_antiwindup) {
    PidController_t pid;
    pid_init(&pid, 0.0f, 1.0f, 0.0f);
    pid_set_limits(&pid, -100.0f, 100.0f, 5.0f);  // Integral limit = 5

    float dt = 1.0f;  // Large dt to quickly hit limit

    // Large error should saturate integral
    for (int i = 0; i < 10; i++) {
        pid_update(&pid, 100.0f, 0.0f, dt);
    }

    // Integral should be clamped at 5
    ASSERT_FLOAT_EQ(pid.integral, 5.0f, 0.001f);
}

TEST(pid_output_clamping) {
    PidController_t pid;
    pid_init(&pid, 10.0f, 0.0f, 0.0f);  // High gain
    pid_set_limits(&pid, -50.0f, 50.0f, 100.0f);

    // Large error should produce clamped output
    float output = pid_update(&pid, 100.0f, 0.0f, 0.02f);
    ASSERT_FLOAT_EQ(output, 50.0f, 0.001f);

    // Negative error
    pid_reset(&pid);
    output = pid_update(&pid, 0.0f, 100.0f, 0.02f);
    ASSERT_FLOAT_EQ(output, -50.0f, 0.001f);
}

TEST(pid_derivative_dampens) {
    PidController_t pid;
    pid_init(&pid, 0.0f, 0.0f, 1.0f);  // Kd=1 only
    pid.use_derivative_on_measurement = true;

    float dt = 0.1f;

    // First update: no previous measurement, d_term should be 0 or small
    float output = pid_update(&pid, 50.0f, 0.0f, dt);
    // d_measurement = (0 - 0) / 0.1 = 0
    ASSERT_FLOAT_EQ(output, 0.0f, 0.001f);

    // Measurement increases: derivative should produce negative output
    // (derivative on measurement with negative sign to dampen)
    output = pid_update(&pid, 50.0f, 10.0f, dt);
    // d_measurement = (10 - 0) / 0.1 = 100
    // d_term = -1.0 * 100 = -100, clamped to -100
    ASSERT_FLOAT_EQ(output, -100.0f, 0.001f);
}

TEST(pid_reset_clears_state) {
    PidController_t pid;
    pid_init(&pid, 1.0f, 1.0f, 1.0f);

    // Run a few updates to accumulate state
    pid_update(&pid, 100.0f, 0.0f, 0.1f);
    pid_update(&pid, 100.0f, 0.0f, 0.1f);

    ASSERT_TRUE(pid.integral != 0.0f);

    pid_reset(&pid);

    ASSERT_FLOAT_EQ(pid.integral, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(pid.prev_error, 0.0f, 0.001f);
}

TEST(pid_convergence) {
    // Test that PID converges to setpoint over time
    PidController_t pid;
    pid_init(&pid, 1.0f, 0.5f, 0.2f);
    pid_set_limits(&pid, -10.0f, 10.0f, 100.0f);

    float setpoint = 100.0f;
    float measurement = 0.0f;
    float dt = 0.02f;  // 50 Hz

    // Simulate a simple first-order system
    // measurement += output * dt * system_gain
    float system_gain = 10.0f;

    for (int i = 0; i < 500; i++) {
        float output = pid_update(&pid, setpoint, measurement, dt);
        measurement += output * dt * system_gain;
    }

    // Should be close to setpoint
    ASSERT_FLOAT_EQ(measurement, setpoint, 5.0f);  // Within 5%
}

// ============================================================
// Test Main
// ============================================================

TEST_MAIN()
    RUN_TEST(pid_init_sets_gains);
    RUN_TEST(pid_init_clears_state);
    RUN_TEST(pid_proportional_only);
    RUN_TEST(pid_integral_accumulates);
    RUN_TEST(pid_integral_antiwindup);
    RUN_TEST(pid_output_clamping);
    RUN_TEST(pid_derivative_dampens);
    RUN_TEST(pid_reset_clears_state);
    RUN_TEST(pid_convergence);
TEST_MAIN_END()
