/*
 * Flat Logger Implementation
 *
 * Single-line, timestamped logs with no nesting/state.
 */

#include "log_manager.h"
#include <stdarg.h>

static bool g_log_manager_begun = false;
static unsigned long g_log_init_time_ms = 0;

// USB CDC enumeration timeout (milliseconds)
// This prevents the device from hanging indefinitely when no USB serial monitor is attached.
// After this timeout, logging will be disabled but the device will continue to boot normally.
#define USB_CDC_TIMEOUT_MS 5000  // 5 seconds

static inline bool serial_ready_for_logging() {
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT == 1)
    // For USB CDC boards (ESP32-C3, C6, S3 with CDCOnBoot=cdc):
    // Wait for USB CDC enumeration, but with a timeout to prevent boot hang.
    // This allows the device to boot normally even when no serial monitor is attached.
    if (!g_log_manager_begun) {
        return false;  // log_init() hasn't been called yet
    }
    
    // Check if Serial is ready (USB CDC enumerated)
    if (Serial) {
        return true;
    }
    
    // If Serial is not ready, check timeout
    // After timeout, disable logging but allow boot to continue
    const unsigned long elapsed = millis() - g_log_init_time_ms;
    return elapsed < USB_CDC_TIMEOUT_MS;
#else
    // For hardware UART boards (classic ESP32):
    // Serial is immediately available after Serial.begin()
    return g_log_manager_begun;
#endif
}

void log_init(unsigned long baud) {
    Serial.begin(baud);
    g_log_manager_begun = true;
    g_log_init_time_ms = millis();  // Record initialization time for timeout calculation
}

static inline char log_level_char(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_ERROR: return 'E';
        case LOG_LEVEL_WARN: return 'W';
        case LOG_LEVEL_INFO: return 'I';
        case LOG_LEVEL_DEBUG: return 'D';
        default: return 'I';
    }
}

void log_write(LogLevel level, const char* module, const char* format, ...) {
    if (!serial_ready_for_logging()) return;
    const unsigned long t = millis();

    char msgbuf[128];
    va_list args;
    va_start(args, format);
    vsnprintf(msgbuf, sizeof(msgbuf), format, args);
    va_end(args);

    char line[200];
    snprintf(line, sizeof(line), "[%lums] %c %s: %s\n", t, log_level_char(level), module, msgbuf);
    Serial.print(line);
}
