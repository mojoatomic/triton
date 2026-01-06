/**
 * RC Submarine Controller
 * display.h - SSD1306 OLED display driver and status screens
 *
 * Power of 10 compliant
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

/*===========================================================================
 * Hardware Configuration
 *===========================================================================*/

#define DISPLAY_I2C_ADDR    0x3C
#define DISPLAY_WIDTH       128
#define DISPLAY_HEIGHT      64
#define DISPLAY_LINES       4
#define DISPLAY_CHARS       21

/*===========================================================================
 * Screen Types
 *===========================================================================*/

typedef enum {
    SCREEN_BOOT,        /* Startup progress */
    SCREEN_READY,       /* Ready to dive */
    SCREEN_DIVING,      /* Underwater stats */
    SCREEN_WARNING,     /* Non-critical warning */
    SCREEN_FAULT,       /* Do not dive */
    SCREEN_EMERGENCY    /* Emergency active */
} ScreenType_t;

typedef enum {
    BOOT_STAGE_CORE1,
    BOOT_STAGE_PRESSURE,
    BOOT_STAGE_IMU,
    BOOT_STAGE_RC,
    BOOT_STAGE_BATTERY,
    BOOT_STAGE_LEAK,
    BOOT_STAGE_COMPLETE,
    BOOT_STAGE_COUNT
} BootStage_t;

typedef enum {
    WARNING_LOW_BATTERY,
    WARNING_WEAK_SIGNAL,
    WARNING_HIGH_PITCH
} WarningType_t;

typedef enum {
    FAULT_PRESSURE_SENSOR,
    FAULT_IMU_SENSOR,
    FAULT_NO_RC_SIGNAL,
    FAULT_CRITICAL_BATTERY,
    FAULT_LEAK_DETECTED,
    FAULT_CORE1_FAILED,
    FAULT_INIT_TIMEOUT
} FaultType_t;

/*===========================================================================
 * Runtime Display Data
 *===========================================================================*/

typedef struct {
    int32_t depth_cm;
    int16_t pitch_deg_x10;
    uint16_t battery_mv;
    bool signal_valid;
    bool depth_hold_active;
} DiveStats_t;

/*===========================================================================
 * Public Functions
 *===========================================================================*/

/**
 * Initialize display hardware
 * Returns: ERR_NONE on success, ERR_I2C on failure
 */
error_t display_init(void);

/**
 * Show boot progress screen
 * stage: Current boot stage
 * success: Whether current stage succeeded
 */
void display_boot_progress(BootStage_t stage, bool success);

/**
 * Show ready to dive screen
 * battery_mv: Current battery voltage in millivolts
 * signal_ok: RC signal status
 */
void display_ready(uint16_t battery_mv, bool signal_ok);

/**
 * Show underwater stats screen
 * stats: Current dive statistics
 */
void display_diving(const DiveStats_t* stats);

/**
 * Show warning screen (still operational)
 * warning: Warning type
 * value: Associated value (e.g., battery voltage)
 */
void display_warning(WarningType_t warning, int32_t value);

/**
 * Show fault screen (do not dive)
 * fault: Fault type
 */
void display_fault(FaultType_t fault);

/**
 * Show emergency screen (flashing)
 * Called repeatedly from main loop
 */
void display_emergency(void);

/**
 * Refresh display hardware
 * Call at 10Hz from Core 1
 */
void display_refresh(void);

#endif /* DISPLAY_H */
