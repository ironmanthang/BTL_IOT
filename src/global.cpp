#include "global.h"

bool sensorData_write(AppContext_t *act, float temp, float humidity, DisplayState_t state) {
    if (act->xMutexSensorData != NULL) {
        if (xSemaphoreTake(act->xMutexSensorData, pdMS_TO_TICKS(100)) == pdTRUE) {
            act->sensorData.temperature = temp;
            act->sensorData.humidity = humidity;
            act->sensorData.state = state;
            xSemaphoreGive(act->xMutexSensorData);
            return true;
        }
    }
    return false;
}

bool sensorData_read(AppContext_t *act, SensorData_t *outData) {
    if (act->xMutexSensorData != NULL && outData != NULL) {
        if (xSemaphoreTake(act->xMutexSensorData, pdMS_TO_TICKS(100)) == pdTRUE) {
            *outData = act->sensorData;
            xSemaphoreGive(act->xMutexSensorData);
            return true;
        }
    }
    return false;
}

void sensorData_update_ai(AppContext_t *act, float ai_score) {
    if (act->xMutexSensorData != NULL) {
        if (xSemaphoreTake(act->xMutexSensorData, pdMS_TO_TICKS(100)) == pdTRUE) {
            act->sensorData.ai_score = ai_score; 
            xSemaphoreGive(act->xMutexSensorData);
        }
    }
}

void setControlMode(AppContext_t *act, ControlMode_t mode) {
    act->controlMode = mode;
}

ControlMode_t getControlMode(AppContext_t *act) {
    return act->controlMode;
}

void setLed1State(AppContext_t *act, bool isOn) {
    portENTER_CRITICAL(&(act->lightStateMux));
    act->led1State = isOn;
    portEXIT_CRITICAL(&(act->lightStateMux));
}

void setLed2State(AppContext_t *act, bool isOn) {
    portENTER_CRITICAL(&(act->lightStateMux));
    act->led2State = isOn;
    portEXIT_CRITICAL(&(act->lightStateMux));
}

void setLed2Color(AppContext_t *act, uint8_t r, uint8_t g, uint8_t b) {
    portENTER_CRITICAL(&(act->lightStateMux));
    act->led2Color[0] = r;
    act->led2Color[1] = g;
    act->led2Color[2] = b;
    act->led2State = (r > 0 || g > 0 || b > 0);
    portEXIT_CRITICAL(&(act->lightStateMux));
}

void getLightStates(AppContext_t *act, bool *led1On, bool *led2On, uint8_t *r, uint8_t *g, uint8_t *b) {
    portENTER_CRITICAL(&(act->lightStateMux));
    if (led1On != NULL) *led1On = act->led1State;
    if (led2On != NULL) *led2On = act->led2State;
    if (r != NULL) *r = act->led2Color[0];
    if (g != NULL) *g = act->led2Color[1];
    if (b != NULL) *b = act->led2Color[2];
    portEXIT_CRITICAL(&(act->lightStateMux));
}