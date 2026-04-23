#include "global.h"

// Cấp phát con trỏ RTOS
SemaphoreHandle_t xMutexSensorData = NULL;
SemaphoreHandle_t xSemaphoreStateChange = NULL;
SemaphoreHandle_t xSemaphoreNeoChange = NULL;
SemaphoreHandle_t xBinarySemaphoreInternet = NULL;

volatile ControlMode_t gControlMode = CONTROL_MODE_AUTO;
volatile bool gLed1State = false;
volatile bool gLed2State = false;
volatile uint8_t gLed2R = 0;
volatile uint8_t gLed2G = 0;
volatile uint8_t gLed2B = 0;

static portMUX_TYPE gLightStateMux = portMUX_INITIALIZER_UNLOCKED;

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

void setControlMode(ControlMode_t mode) {
    gControlMode = mode;
}

ControlMode_t getControlMode() {
    return gControlMode;
}

void setLed1State(bool isOn) {
    portENTER_CRITICAL(&gLightStateMux);
    gLed1State = isOn;
    portEXIT_CRITICAL(&gLightStateMux);
}

void setLed2State(bool isOn) {
    portENTER_CRITICAL(&gLightStateMux);
    gLed2State = isOn;
    portEXIT_CRITICAL(&gLightStateMux);
}

void setLed2Color(uint8_t r, uint8_t g, uint8_t b) {
    portENTER_CRITICAL(&gLightStateMux);
    gLed2R = r;
    gLed2G = g;
    gLed2B = b;
    gLed2State = (r > 0 || g > 0 || b > 0);
    portEXIT_CRITICAL(&gLightStateMux);
}

void getLightStates(bool *led1On, bool *led2On, uint8_t *r, uint8_t *g, uint8_t *b) {
    portENTER_CRITICAL(&gLightStateMux);
    if (led1On != NULL) *led1On = gLed1State;
    if (led2On != NULL) *led2On = gLed2State;
    if (r != NULL) *r = gLed2R;
    if (g != NULL) *g = gLed2G;
    if (b != NULL) *b = gLed2B;
    portEXIT_CRITICAL(&gLightStateMux);
}

