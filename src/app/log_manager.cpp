/*
 * Flat Logger Implementation
 *
 * Single-line, timestamped logs with no nesting/state.
 */

#include "log_manager.h"
#include <stdarg.h>

static bool g_log_manager_begun = false;
static unsigned long g_log_init_time_ms = 0;
static bool g_usb_cdc_logging_disabled = false;

// USB CDC enumeration/consumer timeout (milliseconds)
// This prevents the device from hanging or WDT-resetting when no USB serial monitor is attached
// (or when the host enumerates CDC but doesn't consume output).
// After this timeout, USB CDC logging is disabled for the remainder of the boot.
#define USB_CDC_TIMEOUT_MS 1000  // 1 second

static inline bool serial_ready_for_logging() {
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT == 1)
    // For USB CDC boards (ESP32-C3, C6, S3 with CDCOnBoot=cdc):
    // Avoid blocking writes when no host/terminal is consuming USB CDC data.
    // We gate logging on writable TX space; after a timeout window, we disable logging.
    if (!g_log_manager_begun) {
        return false;  // log_init() hasn't been called yet
    }

    if (g_usb_cdc_logging_disabled) {
        return false;
    }
    
    const int writable = Serial.availableForWrite();
    if (writable > 0) {
        return true;
    }

    // No writable space (typically: no terminal open / host not consuming). After timeout,
    // permanently disable USB CDC logging to guarantee boot progress.
    const unsigned long elapsed = millis() - g_log_init_time_ms;
    if (elapsed >= USB_CDC_TIMEOUT_MS) {
        g_usb_cdc_logging_disabled = true;
    }
    return false;
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
    g_usb_cdc_logging_disabled = false;
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

#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT == 1)
    // Keep USB CDC logging non-blocking: only write if the whole line fits.
    const size_t len = strnlen(line, sizeof(line));
    const int writable = Serial.availableForWrite();
    if (writable <= 0 || (size_t)writable < len) {
        const unsigned long elapsed = millis() - g_log_init_time_ms;
        if (elapsed >= USB_CDC_TIMEOUT_MS) {
            g_usb_cdc_logging_disabled = true;
        }
        return;
    }
    Serial.write((const uint8_t*)line, len);
#else
    Serial.print(line);
#endif
}
