/**
 * RC Submarine Controller
 * log.c - Event logging
 *
 * Power of 10 compliant
 */

#include "log.h"

#include <stddef.h>

static uint8_t next_index(uint8_t idx) {
    P10_ASSERT(idx < EVENT_LOG_SIZE);

    idx++;
    if (idx >= EVENT_LOG_SIZE) {
        idx = 0;
    }

    return idx;
}

static uint8_t wrap_sub(uint8_t a, uint8_t b) {
    P10_ASSERT(a < EVENT_LOG_SIZE);
    P10_ASSERT(b < EVENT_LOG_SIZE);

    if (a >= b) {
        return (uint8_t)(a - b);
    }

    return (uint8_t)(EVENT_LOG_SIZE - (b - a));
}

void log_init(EventLog_t* log) {
    P10_ASSERT(log != NULL);

    if (log == NULL) {
        return;
    }

    for (uint8_t i = 0; i < EVENT_LOG_SIZE; i++) {
        log->entries[i].timestamp_ms = 0U;
        log->entries[i].code = EVT_NONE;
        log->entries[i].param1 = 0U;
        log->entries[i].param2 = 0U;
    }

    log->head = 0U;
    log->count = 0U;
}

void log_event(EventLog_t* log, uint32_t timestamp_ms, EventCode_t code, uint8_t param1, uint8_t param2) {
    P10_ASSERT(log != NULL);

    if (log == NULL) {
        return;
    }

    log->entries[log->head].timestamp_ms = timestamp_ms;
    log->entries[log->head].code = code;
    log->entries[log->head].param1 = param1;
    log->entries[log->head].param2 = param2;

    log->head = next_index(log->head);

    if (log->count < EVENT_LOG_SIZE) {
        log->count++;
    }
}

bool log_get_newest(const EventLog_t* log, uint8_t index_from_newest, EventLogEntry_t* entry_out) {
    P10_ASSERT(log != NULL);
    P10_ASSERT(entry_out != NULL);

    if ((log == NULL) || (entry_out == NULL)) {
        return false;
    }

    if (index_from_newest >= log->count) {
        return false;
    }

    // Newest entry is at head-1
    const uint8_t newest_idx = wrap_sub(log->head, 1U);
    const uint8_t idx = wrap_sub(newest_idx, index_from_newest);

    *entry_out = log->entries[idx];
    return true;
}

uint8_t log_count(const EventLog_t* log) {
    P10_ASSERT(log != NULL);

    if (log == NULL) {
        return 0U;
    }

    return log->count;
}
