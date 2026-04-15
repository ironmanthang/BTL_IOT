#include "global.h"

// Cấp phát con trỏ RTOS
SemaphoreHandle_t xMutexSensorData = NULL;
SemaphoreHandle_t xSemaphoreStateChange = NULL;
SemaphoreHandle_t xSemaphoreNeoChange = NULL;
SemaphoreHandle_t xBinarySemaphoreInternet = NULL;

String WIFI_SSID = "";
String WIFI_PASS = "";
String CORE_IOT_TOKEN = "";
String CORE_IOT_SERVER = "mqtt.thingsboard.cloud";
String CORE_IOT_PORT = "1883";
boolean isWifiConnected = false;

static SensorData_t sharedSensorData = { 0.0f, 0.0f, STATE_NORMAL };

bool sensorData_write(float temp, float humidity, DisplayState_t state) {
    if (xMutexSensorData != NULL) {
        if (xSemaphoreTake(xMutexSensorData, pdMS_TO_TICKS(100)) == pdTRUE) {
            sharedSensorData.temperature = temp;
            sharedSensorData.humidity = humidity;
            sharedSensorData.state = state;
            xSemaphoreGive(xMutexSensorData);
            return true;
        }
    }
    return false;
}

bool sensorData_read(SensorData_t *outData) {
    if (xMutexSensorData != NULL && outData != NULL) {
        if (xSemaphoreTake(xMutexSensorData, pdMS_TO_TICKS(100)) == pdTRUE) {
            *outData = sharedSensorData;
            xSemaphoreGive(xMutexSensorData);
            return true;
        }
    }
    return false;
}

