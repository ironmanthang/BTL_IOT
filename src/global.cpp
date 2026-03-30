#include "global.h"

// ---------------------------------------------------------------------------
// Private shared data — access ONLY via sensorData_write() / sensorData_read()
// ---------------------------------------------------------------------------
static SensorData_t sharedSensorData = { 0.0f, 0.0f, STATE_NORMAL };

// Mutex protecting sharedSensorData
SemaphoreHandle_t xMutexSensorData = xSemaphoreCreateMutex();

// Binary semaphore — given when display state changes, so other tasks can react
SemaphoreHandle_t xSemaphoreStateChange = xSemaphoreCreateBinary();

// ---------------------------------------------------------------------------
// sensorData_write — called by temp_humi_monitor task only
// Acquires mutex, updates struct, releases mutex.
// Gives xSemaphoreStateChange if the display state changed.
// Returns false if mutex could not be acquired within 100ms (deadlock guard).
// ---------------------------------------------------------------------------
bool sensorData_write(float temp, float humidity, DisplayState_t state) {
    if (xSemaphoreTake(xMutexSensorData, pdMS_TO_TICKS(100)) == pdTRUE) {
        DisplayState_t oldState = sharedSensorData.state;
        sharedSensorData.temperature = temp;
        sharedSensorData.humidity    = humidity;
        sharedSensorData.state       = state;
        xSemaphoreGive(xMutexSensorData);

        // Signal state transition to other tasks (LED, NeoPixel, etc.)
        if (state != oldState) {
            xSemaphoreGive(xSemaphoreStateChange);
        }
        return true;
    }
    Serial.println("[global] ERROR: sensorData_write mutex timeout!");
    return false;
}

// ---------------------------------------------------------------------------
// sensorData_read — called by any consumer task (webserver, coreiot, tinyml)
// Acquires mutex, copies struct to outData, releases mutex.
// Returns false if mutex could not be acquired within 100ms (deadlock guard).
// ---------------------------------------------------------------------------
bool sensorData_read(SensorData_t *outData) {
    if (outData == nullptr) return false;
    if (xSemaphoreTake(xMutexSensorData, pdMS_TO_TICKS(100)) == pdTRUE) {
        *outData = sharedSensorData;  // struct copy — safe because mutex is held
        xSemaphoreGive(xMutexSensorData);
        return true;
    }
    Serial.println("[global] ERROR: sensorData_read mutex timeout!");
    return false;
}

// ---------------------------------------------------------------------------
// Wi-Fi / CoreIOT config
// ---------------------------------------------------------------------------
String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

String ssid          = "ESP32-YOUR NETWORK HERE!!!";
String password      = "12345678";
String wifi_ssid     = "abcde";
String wifi_password = "123456789";

boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();