/**
 * RC Submarine Controller
 * safety_monitor.c - Core 0 safety monitor (100Hz)
 *
 * Power of 10 compliant
 */

#include "safety_monitor.h"
#include "emergency.h"
#include "config.h"
#include "battery.h"
#include "leak.h"
#include "log.h"
#include "hardware/watchdog.h"
#include "hardware/gpio.h"
#include "pico/time.h"

// Shared state with Core 1 (volatile for cross-core visibility)
static volatile FaultFlags_t g_faults = {0};
static volatile bool g_emergency = false;
static volatile uint32_t g_last_rc_valid_ms = 0;
static volatile int32_t g_current_depth_cm = 0;
static volatile int16_t g_current_pitch_x10 = 0;

// Core 1 health monitoring
static uint32_t s_last_heartbeat = 0;
static uint32_t s_stall_count = 0;
#define CORE1_STALL_THRESHOLD 10  // 100ms at 100Hz

// Core 1 interface functions
void safety_update_rc_time(uint32_t ms) {
    P10_ASSERT(ms <= 0xFFFFFFFF);
    g_last_rc_valid_ms = ms;
}

void safety_update_depth(int32_t depth_cm) {
    P10_ASSERT(depth_cm >= 0);
    P10_ASSERT(depth_cm <= 10000);
    g_current_depth_cm = depth_cm;
}

void safety_update_pitch(int16_t pitch_x10) {
    P10_ASSERT(pitch_x10 >= -1800);
    P10_ASSERT(pitch_x10 <= 1800);
    g_current_pitch_x10 = pitch_x10;
}

void safety_monitor_init(void) {
    P10_ASSERT(WATCHDOG_TIMEOUT_MS > 0);

    // Enable hardware watchdog (1 second timeout)
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);

    // Initialize LED for heartbeat
    gpio_init(PIN_LED_STATUS);
    gpio_set_dir(PIN_LED_STATUS, GPIO_OUT);

    g_faults.all = 0;
    g_emergency = false;
}

// Helper functions
static void check_rc_signal(uint32_t now_ms) {
    P10_ASSERT(SIGNAL_TIMEOUT_MS > 0);
    if ((now_ms - g_last_rc_valid_ms) > SIGNAL_TIMEOUT_MS) {
        if (!g_faults.bits.signal_lost) {
            g_faults.bits.signal_lost = 1;
            log_event(EVT_SIGNAL_LOST, 0, 0);
        }
    } else {
        if (g_faults.bits.signal_lost) {
            g_faults.bits.signal_lost = 0;
            log_event(EVT_SIGNAL_RESTORED, 0, 0);
        }
    }
}

static void check_battery(void) {
    P10_ASSERT(MIN_BATTERY_MV > 0);

    uint16_t batt_mv = battery_read_mv();
    if (batt_mv < MIN_BATTERY_MV) {
        if (!g_faults.bits.low_battery) {
            g_faults.bits.low_battery = 1;
            log_event(EVT_LOW_BATTERY, batt_mv >> 8, batt_mv & 0xFF);
        }
    }
}

static void check_sensors(void) {
    P10_ASSERT(MAX_DEPTH_CM > 0);
    P10_ASSERT(MAX_PITCH_DEG > 0);
    // Check leak
    if (leak_detected()) {
        if (!g_faults.bits.leak) {
            g_faults.bits.leak = 1;
            log_event(EVT_LEAK_DETECTED, 0, 0);
        }
    }

    // Check depth
    if (g_current_depth_cm > MAX_DEPTH_CM) {
        if (!g_faults.bits.depth_exceeded) {
            g_faults.bits.depth_exceeded = 1;
            log_event(EVT_DEPTH_EXCEEDED, g_current_depth_cm >> 8,
                      g_current_depth_cm & 0xFF);
        }
    }

    // Check pitch
    int16_t pitch_deg = g_current_pitch_x10 / 10;
    if (ABS(pitch_deg) > MAX_PITCH_DEG) {
        if (!g_faults.bits.pitch_exceeded) {
            g_faults.bits.pitch_exceeded = 1;
            log_event(EVT_PITCH_EXCEEDED, pitch_deg, 0);
        }
    }
}

static void check_core1_health(void) {
    P10_ASSERT(CORE1_STALL_THRESHOLD > 0);

    if (g_core1_heartbeat == s_last_heartbeat) {
        s_stall_count++;
        if (s_stall_count > CORE1_STALL_THRESHOLD) {
            if (!g_faults.bits.core1_stall) {
                g_faults.bits.core1_stall = 1;
                log_event(EVT_CORE1_STALL, 0, 0);
                trigger_emergency_blow(EVT_CORE1_STALL);
            }
        }
    } else {
        s_stall_count = 0;
        g_faults.bits.core1_stall = 0;  // Clear fault if heartbeat resumes
    }
    s_last_heartbeat = g_core1_heartbeat;
}

static void update_heartbeat_led(uint32_t now_ms) {
    P10_ASSERT(PIN_LED_STATUS < 30);  // Valid GPIO pin

    static uint32_t last_led_toggle_ms = 0;
    static bool led_state = false;

    uint32_t blink_rate = g_emergency ? 100 : 500;  // Fast blink if emergency
    if ((now_ms - last_led_toggle_ms) >= blink_rate) {
        led_state = !led_state;
        gpio_put(PIN_LED_STATUS, led_state);
        last_led_toggle_ms = now_ms;
    }
}

void safety_monitor_run(void) {
    P10_ASSERT(g_faults.all < 0x10000);  // Valid fault flags

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // 1. Feed watchdog (must happen every loop)
    watchdog_update();

    // 2. Check all fault conditions
    check_rc_signal(now_ms);
    check_battery();
    check_sensors();
    check_core1_health();

    // 3. Trigger emergency if any critical fault
    // Critical faults: signal_lost, low_battery, leak, depth_exceeded, pitch_exceeded, core1_stall
    uint16_t critical_mask = 0x011F;  // First 5 bits + core1_stall (bit 8)
    if ((g_faults.all & critical_mask) && !g_emergency) {
        trigger_emergency_blow(EVT_EMERGENCY_BLOW);
        g_emergency = true;
    }

    // 4. Update heartbeat LED
    update_heartbeat_led(now_ms);
}

FaultFlags_t safety_monitor_get_faults(void) {
    return g_faults;
}

bool safety_monitor_is_emergency(void) {
    return g_emergency;
}
