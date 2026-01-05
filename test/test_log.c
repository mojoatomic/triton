/**
 * Unit Tests: Event Log
 *
 * Tests the fixed-size ring buffer logger.
 * Run with: ./test/run_tests.sh test_log
 */

#include "test_harness.h"
#include "mocks/mock_pico.h"

#include "util/log.h"

TEST(log_init_empty) {
    EventLog_t log;
    EventLogEntry_t e;

    log_init(&log);

    ASSERT_EQ(0, (int)log_count(&log));
    ASSERT_FALSE(log_get_newest(&log, 0, &e));
}

TEST(log_event_ordering) {
    EventLog_t log;
    EventLogEntry_t e;

    log_init(&log);

    log_event(&log, 10U, EVT_BOOT, 1U, 2U);
    log_event(&log, 20U, EVT_MODE_CHANGE, 3U, 4U);

    ASSERT_EQ(2, (int)log_count(&log));

    ASSERT_TRUE(log_get_newest(&log, 0, &e));
    ASSERT_EQ((int)EVT_MODE_CHANGE, (int)e.code);
    ASSERT_EQ(20, (int)e.timestamp_ms);

    ASSERT_TRUE(log_get_newest(&log, 1, &e));
    ASSERT_EQ((int)EVT_BOOT, (int)e.code);
    ASSERT_EQ(10, (int)e.timestamp_ms);
}

TEST(log_wraparound_overwrites_oldest) {
    EventLog_t log;
    EventLogEntry_t e;

    log_init(&log);

    // Write more than the buffer holds
    for (uint8_t i = 0; i < (uint8_t)(EVENT_LOG_SIZE + 2); i++) {
        log_event(&log, (uint32_t)i, EVT_STATE_CHANGE, i, 0U);
    }

    ASSERT_EQ((int)EVENT_LOG_SIZE, (int)log_count(&log));

    // Newest should be the last written
    ASSERT_TRUE(log_get_newest(&log, 0, &e));
    ASSERT_EQ((int)EVT_STATE_CHANGE, (int)e.code);
    ASSERT_EQ((int)(EVENT_LOG_SIZE + 1), (int)e.timestamp_ms);

    // Oldest retained should be timestamp 2
    ASSERT_TRUE(log_get_newest(&log, (uint8_t)(EVENT_LOG_SIZE - 1), &e));
    ASSERT_EQ(2, (int)e.timestamp_ms);
}

TEST_MAIN()
    RUN_TEST(log_init_empty);
    RUN_TEST(log_event_ordering);
    RUN_TEST(log_wraparound_overwrites_oldest);
TEST_MAIN_END()
