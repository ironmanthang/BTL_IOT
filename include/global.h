#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ---------------------------------------------------------------------------
// Sensor Display State
// ---------------------------------------------------------------------------
typedef enum {
    STATE_NORMAL   = 0,  // temp < 29°C AND humidity < 65%
    STATE_WARNING  = 1,  // temp 29-35°C OR humidity 65-80%
    STATE_CRITICAL = 2   // temp > 35°C OR humidity > 80%
} DisplayState_t;

// ---------------------------------------------------------------------------
// Shared Sensor Data Struct — NEVER access sharedSensorData directly.
// Always use sensorData_write() / sensorData_read() which are mutex-protected.
// ---------------------------------------------------------------------------
typedef struct {
    float          temperature;  // °C
    float          humidity;     // %
    DisplayState_t state;        // current display state
} SensorData_t;

// Mutex protecting sharedSensorData (created in global.cpp)
extern SemaphoreHandle_t xMutexSensorData;

// Binary semaphore — given whenever the display state CHANGES (NORMAL→WARNING etc.)
// Other tasks (LED, NeoPixel) can xSemaphoreTake() this to react to state transitions.
extern SemaphoreHandle_t xSemaphoreStateChange;

// ---------------------------------------------------------------------------
// Accessor API — use these everywhere instead of global variables
// Returns true on success, false if mutex could not be acquired within 100ms.
// ---------------------------------------------------------------------------
bool sensorData_write(float temp, float humidity, DisplayState_t state);
bool sensorData_read(SensorData_t *outData);

// ---------------------------------------------------------------------------
// Wi-Fi / CoreIOT config (kept as-is — not sensor data)
// ---------------------------------------------------------------------------
extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

#endif // __GLOBAL_H__