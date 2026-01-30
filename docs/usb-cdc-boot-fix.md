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

The fix is to make USB CDC logging **non-blocking** and to **give up quickly** if the host isn’t consuming output.

On USB CDC boards (`CDCOnBoot=cdc`), we:

1. **Gate logging on TX space**: only attempt a write when `Serial.availableForWrite()` reports there is room.
2. **Avoid partial writes**: only write a log line if the entire line fits in the available TX space; otherwise drop it.
3. **Fail-safe disable**: if no TX space becomes available within `USB_CDC_TIMEOUT_MS` (now 1000ms), disable USB CDC logging for the remainder of that boot.

This prevents watchdog resets / boot hangs caused by blocking USB CDC writes when no serial monitor is open.

In addition, the old boot-time `delay(1000)` after `log_init()` was removed from `app.ino` because it wasn’t solving the root cause.

## Behavior After Fix

### With Serial Monitor Attached
- USB CDC enumerates quickly (usually <1 second)
- All logs are captured normally
- Boot completes in ~2-3 seconds

### Without Serial Monitor Attached
- Device waits up to 1 second for USB CDC
- After timeout, logging is silently disabled
- Boot continues normally
- Device functions properly for standalone deployment

### Log Output Timing

- Logs that can’t be written immediately are **dropped** (no buffering).
- If a serial monitor is open and consuming output, logs appear normally.
- If no monitor is consuming output for the first ~1 second, USB CDC logging becomes a no-op for that boot.

## Benefits
- Host consumes USB CDC output
- Logs are printed normally
2. **No Boot Delays**: Minimal impact when USB CDC not available
3. **Maintains Logging**: Full logging support when USB CDC is available
- Device does not block on logging
- After 1 second of no writable TX space, USB CDC logging is disabled

## Alternative Solutions Considered

### Log Output Timing

We intentionally drop logs when USB CDC can’t accept data to keep boot deterministic and avoid watchdog resets.
### Option 3: Shorter timeout (e.g., 1 second)
- Fast boot and still captures logs when a monitor is already open
- If your host enumerates slowly, you may miss the earliest boot logs

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
