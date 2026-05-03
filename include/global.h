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

typedef struct {
    // Các con trỏ đồng bộ RTOS
    SemaphoreHandle_t xMutexSensorData;
    SemaphoreHandle_t xSemaphoreStateChange; 
    SemaphoreHandle_t xSemaphoreNeoChange;   
    SemaphoreHandle_t xBinarySemaphoreInternet; 
    
    // Phần cứng
    Adafruit_SSD1306 * lcd;
    DHT20 * dht;
    Adafruit_NeoPixel* pixels;

    // Dữ liệu chung
    SensorData_t sensorData;
    portMUX_TYPE lightStateMux; 

    // CHUYỂN TOÀN BỘ BIẾN TOÀN CỤC VÀO ĐÂY
    String wifi_ssid;
    String wifi_pass;
    String core_iot_token;
    String core_iot_server;
    String core_iot_port;
    
    ControlMode_t controlMode;
    bool led1State;
    bool led2State;
    uint8_t led2Color[3];
} AppContext_t;

// Các hàm Helper (bắt buộc truyền act vào để xử lý)
bool sensorData_write(AppContext_t *act, float temp, float humidity, DisplayState_t state);
bool sensorData_read(AppContext_t *act, SensorData_t *outData);
void sensorData_update_ai(AppContext_t *act, float ai_score);

void setControlMode(AppContext_t *act, ControlMode_t mode);
ControlMode_t getControlMode(AppContext_t *act);
void setLed1State(AppContext_t *act, bool isOn);
void setLed2State(AppContext_t *act, bool isOn);
void setLed2Color(AppContext_t *act, uint8_t r, uint8_t g, uint8_t b);
void getLightStates(AppContext_t *act, bool *led1On, bool *led2On, uint8_t *r, uint8_t *g, uint8_t *b);

#endif