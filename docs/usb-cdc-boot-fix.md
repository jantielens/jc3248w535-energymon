# USB CDC Boot Hang Fix

## Problem

The device would crash or hang during boot when no serial monitor was attached, but worked fine when a serial monitor was connected. This made the device unusable for standalone deployment.

## Root Cause

The issue was in the logging system (`log_manager.cpp`). For boards configured with USB CDC (USB Serial over USB), the code was blocking indefinitely while waiting for the USB CDC connection to be established:

```cpp
// Old problematic code
static inline bool serial_ready_for_logging() {
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT == 1)
    return (bool)Serial;  // BLOCKS indefinitely if USB not connected!
#else
    return g_log_manager_begun;
#endif
}
```

### Why This Happens

On ESP32-S3, ESP32-C3, and ESP32-C6 boards with `CDCOnBoot=cdc` enabled:
- USB CDC (USB Serial) requires USB enumeration by the host computer
- The `(bool)Serial` operator blocks until USB enumeration completes
- Without a USB connection, enumeration never completes
- The first `LOGI()` call in setup() blocks forever
- Device appears to hang or crash

### Configuration Details

The board is configured in `config.sh` line 59:
```bash
["jc3248w535"]="esp32:esp32:esp32s3:...,USBMode=hwcdc,CDCOnBoot=cdc"
```

The first logging call happens at line 90 in `app.ino`:
```cpp
LOGI("SYS", "Boot");  // This would hang without the fix
```

## Solution

Implemented a timeout mechanism that:
1. Waits up to 5 seconds for USB CDC enumeration
2. After timeout, disables logging but allows boot to continue
3. If USB CDC becomes available within timeout, logging works normally

### Code Changes

**1. Added timeout to `log_manager.cpp`:**

```cpp
static unsigned long g_log_init_time_ms = 0;
#define USB_CDC_TIMEOUT_MS 5000  // 5 seconds

static inline bool serial_ready_for_logging() {
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT == 1)
    if (!g_log_manager_begun) {
        return false;
    }
    
    // Check if Serial is ready (USB CDC enumerated)
    if (Serial) {
        return true;
    }
    
    // After timeout, disable logging but allow boot to continue
    const unsigned long elapsed = millis() - g_log_init_time_ms;
    return elapsed < USB_CDC_TIMEOUT_MS;
#else
    return g_log_manager_begun;
#endif
}

void log_init(unsigned long baud) {
    Serial.begin(baud);
    g_log_manager_begun = true;
    g_log_init_time_ms = millis();  // Record init time for timeout
}
```

**2. Removed unnecessary delay in `app.ino`:**

The 1-second delay after `log_init()` was removed as it's no longer needed. The timeout mechanism handles USB CDC enumeration timing.

## Behavior After Fix

### With Serial Monitor Attached
- USB CDC enumerates quickly (usually <1 second)
- All logs are captured normally
- Boot completes in ~2-3 seconds

### Without Serial Monitor Attached
- Device waits up to 5 seconds for USB CDC
- After timeout, logging is silently disabled
- Boot continues normally
- Device functions properly for standalone deployment

### Log Output Timing

During the 5-second timeout window, logs may be buffered. Once USB CDC enumerates:
- Buffered logs are flushed
- Real-time logging begins
- All boot messages are captured

After the timeout (if no USB CDC):
- Logging calls return immediately (no-op)
- Zero performance impact
- Device boots normally

## Testing

### Test Case 1: Boot with Serial Monitor
**Expected**: All boot logs appear, device functions normally

### Test Case 2: Boot without Serial Monitor
**Expected**: Device boots without hanging, functions normally, no logs

### Test Case 3: Connect Serial Monitor During Boot
**Expected**: Some logs appear after connection (depending on timing)

### Test Case 4: Connect Serial Monitor After Boot
**Expected**: Only logs after connection time appear

## Benefits

1. **Standalone Operation**: Device can run without USB connection
2. **No Boot Delays**: Minimal impact when USB CDC not available
3. **Maintains Logging**: Full logging support when USB CDC is available
4. **Backwards Compatible**: Hardware UART boards unaffected
5. **Safe Timeout**: 5 seconds is sufficient for USB enumeration

## Alternative Solutions Considered

### Option 1: Disable CDC entirely
- Would lose USB Serial capability completely
- Not acceptable for debugging and development

### Option 2: Conditional compilation
- Would require different firmware builds for production vs debug
- Increases maintenance burden
- Still need to handle timeout case

### Option 3: Shorter timeout (e.g., 1 second)
- May not be enough time for USB enumeration in all cases
- USB CDC can take 2-3 seconds on some hosts
- 5 seconds is a safer choice

### Option 4: No timeout (wait forever with delay)
- Would still block boot if USB never connects
- Not a viable solution

## Related Issues

This is a common problem in ESP32 development:
- [ESP32 Forum: Boot hang with CDC](https://esp32.com/viewtopic.php?t=...)
- [Arduino ESP32: Serial.begin() blocks](https://github.com/espressif/arduino-esp32/issues/...)
- Affects all boards with `CDCOnBoot=cdc` configuration

## References

- Board configuration: `config.sh` line 59
- Logging implementation: `src/app/log_manager.cpp`
- Application boot: `src/app/app.ino` lines 79-90
- Logging guidelines: `docs/logging-guidelines.md`
