#ifndef ENERGY_MONITOR_H
#define ENERGY_MONITOR_H

#include <Arduino.h>
#include "config_manager.h"

// Thread-safe state for the Energy Monitor screen.
// Updated from the MQTT loop task; read from the LVGL task.

struct EnergyMonitorState {
    float solar_value;
    float grid_value;
    bool solar_updated;
    bool grid_updated;
    uint32_t solar_update_ms;
    uint32_t grid_update_ms;

    float consumer_values[ENERGY_CONSUMER_COUNT];
    bool consumer_updated[ENERGY_CONSUMER_COUNT];
    uint32_t consumer_update_ms[ENERGY_CONSUMER_COUNT];
};

void energy_monitor_init();

// Record a new value (value may be NAN).
void energy_monitor_set_solar(float value, uint32_t now_ms);
void energy_monitor_set_grid(float value, uint32_t now_ms);
void energy_monitor_set_consumer_value(uint8_t index, float value, uint32_t now_ms);

// Read current state. If clear_updates is true, the *_updated flags are cleared.
EnergyMonitorState energy_monitor_get_state(bool clear_updates);

// True when any category exceeds its configured warning (T2) threshold.
struct DeviceConfig;
bool energy_monitor_has_warning(const DeviceConfig* config);

#endif // ENERGY_MONITOR_H
