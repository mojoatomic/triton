/**
 * RC Submarine Controller
 * display.c - SSD1306 OLED display driver and status screens
 *
 * Power of 10 compliant
 */

#include "display.h"
#include "config.h"
#include "util/log.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdio.h>

/*===========================================================================
 * Constants
 *===========================================================================*/

#define DISPLAY_BUFFER_SIZE     (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)
#define FONT_WIDTH              6
#define FONT_HEIGHT             8
#define LARGE_FONT_WIDTH        12
#define LARGE_FONT_HEIGHT       16

/* SSD1306 Commands */
#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_DISPLAY_ON          0xAF
#define SSD1306_DISPLAY_OFF         0xAE
#define SSD1306_SET_DISP_NORMAL     0xA6
#define SSD1306_SET_DISP_INVERSE    0xA7
#define SSD1306_SET_MEM_ADDR_MODE   0x20
#define SSD1306_SET_COL_ADDR        0x21
#define SSD1306_SET_PAGE_ADDR       0x22

/* Boot stage names */
static const char* const BOOT_STAGE_NAMES[BOOT_STAGE_COUNT] = {
    "Core 1",
    "Pressure sensor",
    "IMU",
    "RC receiver",
    "Battery",
    "Leak sensor",
    "Complete"
};

/*===========================================================================
 * Static Data
 *===========================================================================*/

static uint8_t s_frame_buffer[DISPLAY_BUFFER_SIZE];
static bool s_initialized = false;

/*===========================================================================
 * 5x7 Font Data (ASCII 32-127)
 *===========================================================================*/

static const uint8_t FONT_5X7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x00, 0x7F, 0x41, 0x41}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
    {0x41, 0x41, 0x7F, 0x00, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x00, 0x7F, 0x10, 0x28, 0x44}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}, // ->
    {0x08, 0x1C, 0x2A, 0x08, 0x08}, // <-
};

/*===========================================================================
 * Low-level I2C Functions
 *===========================================================================*/

static void ssd1306_write_cmd(uint8_t cmd) {
    P10_ASSERT(s_initialized || cmd == SSD1306_DISPLAY_ON || cmd == SSD1306_DISPLAY_OFF);

    uint8_t buf[2] = {0x00, cmd};
    int ret = i2c_write_blocking(DISPLAY_I2C, DISPLAY_I2C_ADDR, buf, 2, false);
    (void)ret;  // Ignore errors in release
}

static void ssd1306_write_data(const uint8_t* data, uint32_t len) {
    P10_ASSERT(data != NULL);
    P10_ASSERT(len <= DISPLAY_BUFFER_SIZE);

    // Send data in chunks with control byte
    uint8_t buf[17];  // 1 control + 16 data
    buf[0] = 0x40;  // Co=0, D/C#=1 (data mode)

    for (uint32_t i = 0; i < len; i += 16) {
        uint32_t chunk = (len - i > 16) ? 16 : (len - i);
        for (uint32_t j = 0; j < chunk; j++) {
            buf[j + 1] = data[i + j];
        }
        int ret = i2c_write_blocking(DISPLAY_I2C, DISPLAY_I2C_ADDR, buf, chunk + 1, false);
        (void)ret;
    }
}

/*===========================================================================
 * Framebuffer Functions
 *===========================================================================*/

static void fb_clear(void) {
    P10_ASSERT(DISPLAY_BUFFER_SIZE > 0);
    memset(s_frame_buffer, 0, DISPLAY_BUFFER_SIZE);
}

static void fb_set_pixel(uint8_t x, uint8_t y, bool on) {
    P10_ASSERT(DISPLAY_WIDTH > 0);
    P10_ASSERT(DISPLAY_HEIGHT > 0);

    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
        return;
    }

    uint32_t byte_idx = x + (y / 8) * DISPLAY_WIDTH;
    uint8_t bit_mask = 1 << (y % 8);

    if (on) {
        s_frame_buffer[byte_idx] |= bit_mask;
    } else {
        s_frame_buffer[byte_idx] &= ~bit_mask;
    }
}

static void fb_draw_char(uint8_t x, uint8_t y, char c) {
    P10_ASSERT(FONT_WIDTH > 0);

    if (c < 32) {
        c = '?';
    }

    uint8_t idx = (uint8_t)(c - 32);
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = FONT_5X7[idx][col];
        for (uint8_t row = 0; row < 7; row++) {
            fb_set_pixel(x + col, y + row, (line >> row) & 0x01);
        }
    }
}

static void fb_draw_string(uint8_t x, uint8_t y, const char* str) {
    P10_ASSERT(str != NULL);
    P10_ASSERT(DISPLAY_CHARS > 0);

    uint8_t cursor_x = x;
    size_t len = strnlen(str, DISPLAY_CHARS);

    for (size_t i = 0; i < len; i++) {
        fb_draw_char(cursor_x, y, str[i]);
        cursor_x += FONT_WIDTH;
        if (cursor_x >= DISPLAY_WIDTH) {
            break;
        }
    }
}

static void fb_draw_progress_bar(uint8_t x, uint8_t y, uint8_t width,
                                  uint8_t percent) {
    P10_ASSERT(percent <= 100);

    uint8_t filled = (width * percent) / 100;

    // Draw border
    for (uint8_t i = 0; i < width; i++) {
        fb_set_pixel(x + i, y, true);
        fb_set_pixel(x + i, y + 7, true);
    }
    for (uint8_t j = 0; j < 8; j++) {
        fb_set_pixel(x, y + j, true);
        fb_set_pixel(x + width - 1, y + j, true);
    }

    // Fill progress
    for (uint8_t i = 2; i < filled - 2; i++) {
        for (uint8_t j = 2; j < 6; j++) {
            fb_set_pixel(x + i, y + j, true);
        }
    }
}

static void fb_draw_large_char(uint8_t x, uint8_t y, char c) {
    P10_ASSERT(LARGE_FONT_WIDTH > 0);

    if (c < 32 || c > 127) {
        c = '?';
    }

    // Scale 5x7 font to 10x14
    uint8_t idx = (uint8_t)(c - 32);
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = FONT_5X7[idx][col];
        for (uint8_t row = 0; row < 7; row++) {
            bool on = (line >> row) & 0x01;
            // Draw 2x2 block for each pixel
            uint32_t byte_idx = (x + col * 2) + ((y + row * 2) / 8) * DISPLAY_WIDTH;
            if (byte_idx < DISPLAY_BUFFER_SIZE && on) {
                fb_set_pixel(x + col * 2, y + row * 2, true);
                fb_set_pixel(x + col * 2 + 1, y + row * 2, true);
                fb_set_pixel(x + col * 2, y + row * 2 + 1, true);
                fb_set_pixel(x + col * 2 + 1, y + row * 2 + 1, true);
            }
        }
    }
}

static void fb_draw_large_string(uint8_t x, uint8_t y, const char* str) {
    P10_ASSERT(str != NULL);
    P10_ASSERT(DISPLAY_CHARS > 0);

    uint8_t cursor_x = x;
    size_t len = strnlen(str, DISPLAY_CHARS);

    for (size_t i = 0; i < len; i++) {
        fb_draw_large_char(cursor_x, y, str[i]);
        cursor_x += LARGE_FONT_WIDTH;
        if (cursor_x >= DISPLAY_WIDTH) {
            break;
        }
    }
}

/*===========================================================================
 * Public API
 *===========================================================================*/

void display_init(void) {
    P10_ASSERT(DISPLAY_I2C_ADDR > 0);

    // Initialize I2C if not already done
    i2c_init(DISPLAY_I2C, 400000);

    // SSD1306 initialization sequence
    ssd1306_write_cmd(SSD1306_DISPLAY_OFF);
    ssd1306_write_cmd(SSD1306_SET_MEM_ADDR_MODE);
    ssd1306_write_cmd(0x00);  // Horizontal addressing
    ssd1306_write_cmd(SSD1306_SET_DISP_NORMAL);
    ssd1306_write_cmd(SSD1306_SET_CONTRAST);
    ssd1306_write_cmd(0x7F);  // Medium contrast
    ssd1306_write_cmd(SSD1306_DISPLAY_ON);

    s_initialized = true;
    fb_clear();
}

void display_boot_progress(BootStage_t stage) {
    P10_ASSERT(stage < BOOT_STAGE_COUNT);

    if (!s_initialized) {
        return;
    }

    fb_clear();

    // Title
    fb_draw_string(20, 0, "RC SUB BOOT");

    // Progress bar
    uint8_t percent = ((uint8_t)stage * 100) / BOOT_STAGE_COUNT;
    fb_draw_progress_bar(10, 16, 108, percent);

    // Stage name
    if (stage < BOOT_STAGE_COUNT) {
        fb_draw_string(0, 28, BOOT_STAGE_NAMES[stage]);
    }

    // Status dots
    for (uint8_t i = 0; i <= (uint8_t)stage && i < BOOT_STAGE_COUNT; i++) {
        fb_set_pixel(10 + i * 8, 40, true);
        fb_set_pixel(11 + i * 8, 40, true);
        fb_set_pixel(10 + i * 8, 41, true);
        fb_set_pixel(11 + i * 8, 41, true);
    }

    display_refresh();
}

void display_ready(void) {
    P10_ASSERT(s_initialized);

    if (!s_initialized) {
        return;
    }

    fb_clear();

    // Large "READY" text
    fb_draw_large_string(24, 8, "READY");

    // Decorative line
    for (uint8_t i = 10; i < 118; i++) {
        fb_set_pixel(i, 30, true);
    }

    // Status info
    fb_draw_string(0, 40, "Systems nominal");
    fb_draw_string(0, 50, "Awaiting RC signal");

    display_refresh();
}

void display_status(const DisplayStatus_t* status) {
    P10_ASSERT(status != NULL);

    if (!s_initialized || status == NULL) {
        return;
    }

    fb_clear();

    char line[22];

    // Line 1: Depth
    snprintf(line, sizeof(line), "Depth: %ld cm", (long)status->depth_cm);
    fb_draw_string(0, 0, line);

    // Line 2: Battery
    snprintf(line, sizeof(line), "Batt: %u mV", status->battery_mv);
    fb_draw_string(0, 10, line);

    // Line 3: Pitch
    snprintf(line, sizeof(line), "Pitch: %d deg", status->pitch_deg);
    fb_draw_string(0, 20, line);

    // Line 4: State
    snprintf(line, sizeof(line), "State: %d", status->state);
    fb_draw_string(0, 30, line);

    // Line 5: RC signal indicator
    if (status->rc_connected) {
        fb_draw_string(0, 40, "RC: Connected");
    } else {
        fb_draw_string(0, 40, "RC: LOST!");
    }

    // Line 6: Faults
    if (status->faults != 0) {
        snprintf(line, sizeof(line), "FAULT: 0x%04X", status->faults);
        fb_draw_string(0, 50, line);
    }

    display_refresh();
}

void display_warning(WarningType_t type, int32_t value) {
    P10_ASSERT(type < WARNING_COUNT);

    if (!s_initialized) {
        return;
    }

    fb_clear();

    // Warning header
    fb_draw_large_string(16, 0, "WARNING");

    char line1[22];
    char line2[22];

    switch (type) {
        case WARNING_LOW_BATTERY:
            snprintf(line1, sizeof(line1), "Low battery");
            snprintf(line2, sizeof(line2), "%ld mV", (long)value);
            break;
        case WARNING_SIGNAL_LOST:
            snprintf(line1, sizeof(line1), "Signal lost");
            snprintf(line2, sizeof(line2), "%ld ms ago", (long)value);
            break;
        case WARNING_DEPTH_LIMIT:
            snprintf(line1, sizeof(line1), "Depth limit");
            snprintf(line2, sizeof(line2), "%ld cm", (long)value);
            break;
        case WARNING_PITCH_LIMIT:
            snprintf(line1, sizeof(line1), "Pitch: %ld", (long)(value / 10));
            line2[0] = '\0';
            break;
        default:
            snprintf(line1, sizeof(line1), "Unknown");
            line2[0] = '\0';
            break;
    }

    fb_draw_string(0, 24, line1);
    fb_draw_string(0, 36, line2);

    display_refresh();
}

void display_fault(uint16_t fault_flags) {
    P10_ASSERT(fault_flags <= 0xFFFF);

    if (!s_initialized) {
        return;
    }

    fb_clear();

    // Fault header
    fb_draw_large_string(24, 0, "FAULT");

    char line[22];
    snprintf(line, sizeof(line), "Code: 0x%04X", fault_flags);
    fb_draw_string(0, 24, line);

    // Decode common faults
    uint8_t y = 36;
    if (fault_flags & 0x0001) {
        fb_draw_string(0, y, "Signal lost");
        y += 10;
    }
    if (fault_flags & 0x0002) {
        fb_draw_string(0, y, "Low battery");
        y += 10;
    }
    if (fault_flags & 0x0004) {
        fb_draw_string(0, y, "Leak detected!");
        y += 10;
    }
    if (fault_flags & 0x0008) {
        fb_draw_string(0, y, "Depth exceeded");
        (void)y; // Suppress unused variable warning
    }

    display_refresh();
}

void display_emergency(void) {
    P10_ASSERT(s_initialized);

    if (!s_initialized) {
        return;
    }

    fb_clear();

    // Large emergency text
    fb_draw_large_string(4, 8, "EMERGENCY");

    // Flashing border (solid for now)
    for (uint8_t i = 0; i < DISPLAY_WIDTH; i++) {
        fb_set_pixel(i, 0, true);
        fb_set_pixel(i, 1, true);
        fb_set_pixel(i, DISPLAY_HEIGHT - 2, true);
        fb_set_pixel(i, DISPLAY_HEIGHT - 1, true);
    }
    for (uint8_t j = 0; j < DISPLAY_HEIGHT; j++) {
        fb_set_pixel(0, j, true);
        fb_set_pixel(1, j, true);
        fb_set_pixel(DISPLAY_WIDTH - 2, j, true);
        fb_set_pixel(DISPLAY_WIDTH - 1, j, true);
    }

    fb_draw_string(8, 32, "EMERGENCY BLOW");
    fb_draw_string(20, 44, "SURFACING");

    display_refresh();
}

void display_refresh(void) {
    P10_ASSERT(s_initialized);

    if (!s_initialized) {
        return;
    }

    // Set column address
    ssd1306_write_cmd(SSD1306_SET_COL_ADDR);
    ssd1306_write_cmd(0);
    ssd1306_write_cmd(DISPLAY_WIDTH - 1);

    // Set page address
    ssd1306_write_cmd(SSD1306_SET_PAGE_ADDR);
    ssd1306_write_cmd(0);
    ssd1306_write_cmd((DISPLAY_HEIGHT / 8) - 1);

    // Write buffer
    ssd1306_write_data(s_frame_buffer, DISPLAY_BUFFER_SIZE);
}
