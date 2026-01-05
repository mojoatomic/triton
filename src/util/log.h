/**
 * RC Submarine Controller
 * log.h - Event logging
 *
 * Power of 10 compliant
 */

#ifndef LOG_H
#define LOG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void log_init(EventLog_t* log);
void log_event(EventLog_t* log, uint32_t timestamp_ms, EventCode_t code, uint8_t param1, uint8_t param2);
bool log_get_newest(const EventLog_t* log, uint8_t index_from_newest, EventLogEntry_t* entry_out);
uint8_t log_count(const EventLog_t* log);

#ifdef __cplusplus
}
#endif

#endif // LOG_H
