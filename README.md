# EnergyMon ESP32

EnergyMon is an ESP32-based energy monitor display with a built-in web portal. It shows solar, home, and grid power flows and can highlight optional consumer devices (EV, washer, HVAC, etc.) with icons.

## What it does

- Live energy flow visualization (solar, home, grid)
- Configurable color bands and warning thresholds
- Optional consumer indicators with custom icons
- Built‑in web portal for Wi‑Fi, MQTT, and display settings
- Screen saver and wake-on-MQTT trigger
- OTA firmware updates from the portal

## Getting started

1. Power the device.
2. Connect to the Wi‑Fi AP (shown on the screen) if the device is not configured.
3. Open the portal at http://192.168.4.1
4. Go to **Network** and connect it to your home Wi‑Fi.
5. Open the portal at the device’s IP address or hostname.

## Web portal overview

The portal has three pages:

- **Home**: Energy monitor settings
- **Network**: Wi‑Fi and device network configuration
- **Firmware**: Updates, manual upload, factory reset

### Home page sections

- **Energy Sources (MQTT)**: Set solar and grid topics and value paths.
- **Screen Saver Wake (MQTT)**: Configure a topic/payload that wakes the display.
- **Consumer Indicators (Optional)**: Up to five consumer topics, thresholds, and icons.
- **Energy Bar Chart Ranges**: Set max kW for the bar charts.
- **T2 Warning Pulse & Hysteresis**: Tune alert pulse timing and clearing behavior.
- **Energy Band Colors & Thresholds**: Set color bands and kW thresholds.
- **Display Brightness & Screen Saver**: Backlight brightness and sleep behavior.

## MQTT configuration

### Topics and payloads

- Solar and grid topics can send **numeric payloads** (e.g., `0.92`) or **JSON**.
- Use **Value Path** to select a JSON key (e.g., `value`). Use `.` for raw numeric payloads.

Examples:

- Topic: `home/solar/power` with payload `1.25`
- Topic: `home/grid/power` with payload `{ "value": -0.45 }`

### Units

- Values are in **kW**.
- Grid can be negative (export) or positive (import).

### Consumer indicators

- Each consumer uses a topic + threshold.
- If value > threshold, its icon appears on the display.
- Empty topic disables the consumer.

## Display behavior

- **Color bands**: define the colors for low/medium/high kW ranges.
- **T2 warning**: when a value exceeds T2, the screen pulses using the warning color.
- **Bar chart ranges**: determine the full scale for each bar.

## Firmware updates

Use the **Firmware** page to:

- Check for online updates
- Upload a firmware file manually
- Factory reset the device

## Troubleshooting

- If the portal isn’t reachable, reconnect to the device AP and reconfigure Wi‑Fi.
- If values don’t update, verify MQTT topics and value paths.
- For consumer icons, confirm the topic publishes numbers above the threshold.
- **Boot Issues**: If the device hangs during boot, see [USB CDC Boot Fix](docs/usb-cdc-boot-fix.md). The device is designed to work with or without a serial monitor attached.
