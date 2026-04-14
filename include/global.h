#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// 1. Trạng thái hiển thị
typedef enum {
    STATE_NORMAL   = 0,
    STATE_WARNING  = 1,
    STATE_CRITICAL = 2
} DisplayState_t;

typedef struct {
    float          temperature; 
    float          humidity;     
    DisplayState_t state;        
} SensorData_t;

extern SemaphoreHandle_t xMutexSensorData;
extern SemaphoreHandle_t xSemaphoreStateChange; // Dành cho Task LED
extern SemaphoreHandle_t xSemaphoreNeoChange;   // Dành cho Task NeoPixel
extern SemaphoreHandle_t xBinarySemaphoreInternet; // Dành cho CoreIOT sau này

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;
extern boolean isWifiConnected;

bool sensorData_write(float temp, float humidity, DisplayState_t state);
bool sensorData_read(SensorData_t *outData);

#endif