#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT20.h"
#include <Adafruit_NeoPixel.h>

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

// extern SemaphoreHandle_t xMutexSensorData;
// extern SemaphoreHandle_t xSemaphoreStateChange; // Dành cho Task LED
// extern SemaphoreHandle_t xSemaphoreNeoChange;   // Dành cho Task NeoPixel
// extern SemaphoreHandle_t xBinarySemaphoreInternet; // Dành cho CoreIOT sau này

typedef struct {
    SemaphoreHandle_t xMutexSensorData;
    SemaphoreHandle_t xSemaphoreStateChange; // Dành cho Task LED
    SemaphoreHandle_t xSemaphoreNeoChange;   // Dành cho Task NeoPixel
    SemaphoreHandle_t xBinarySemaphoreInternet; // Dành cho CoreIOT sau này
    
    Adafruit_SSD1306 * lcd;
    DHT20 * dht ;
    Adafruit_NeoPixel* pixels;

    SensorData_t sensorData;

} AppContext_t;

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;
extern boolean isWifiConnected;

bool sensorData_write(float temp, float humidity, DisplayState_t state);
bool sensorData_read(SensorData_t *outData);

#endif