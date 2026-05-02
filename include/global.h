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

typedef enum {
    STATE_NORMAL   = 0,
    STATE_WARNING  = 1,
    STATE_CRITICAL = 2
} DisplayState_t;

typedef enum {
    CONTROL_MODE_AUTO = 0,
    CONTROL_MODE_MANUAL = 1
} ControlMode_t;

typedef struct {
    float          temperature; 
    float          humidity;     
    DisplayState_t state;
    float          ai_score;        
} SensorData_t;

// ĐÃ MỞ KHÓA COMMENT ĐỂ DÙNG CHUNG TOÀN HỆ THỐNG
extern SemaphoreHandle_t xMutexSensorData;
extern SemaphoreHandle_t xSemaphoreStateChange; 
extern SemaphoreHandle_t xSemaphoreNeoChange;   
extern SemaphoreHandle_t xBinarySemaphoreInternet; 

typedef struct {
    SemaphoreHandle_t xMutexSensorData;
    SemaphoreHandle_t xSemaphoreStateChange; 
    SemaphoreHandle_t xSemaphoreNeoChange;   
    SemaphoreHandle_t xBinarySemaphoreInternet; 
    
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

extern volatile ControlMode_t gControlMode;
extern volatile bool gLed1State;
extern volatile bool gLed2State;
extern volatile uint8_t gLed2R;
extern volatile uint8_t gLed2G;
extern volatile uint8_t gLed2B;

bool sensorData_write(float temp, float humidity, DisplayState_t state);
bool sensorData_read(SensorData_t *outData);
void sensorData_update_ai(float ai_score);
void setControlMode(ControlMode_t mode);
ControlMode_t getControlMode();
void setLed1State(bool isOn);
void setLed2State(bool isOn);
void setLed2Color(uint8_t r, uint8_t g, uint8_t b);
void getLightStates(bool *led1On, bool *led2On, uint8_t *r, uint8_t *g, uint8_t *b);

#endif