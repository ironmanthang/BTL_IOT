#include <Arduino.h>
#include <Wire.h>
#include "global.h"
#include <Adafruit_NeoPixel.h>
#include "temp_humi_monitor.h"
#include "led_blinky.h"
#include "neo_blinky.h"
#include "tinyml.h"
#include "coreiot.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_toogle_boot.h"
#include <LittleFS.h> 

void setup() {
    Serial.begin(115200);
    delay(3000); 
    Serial.println("\n\n===== HỆ THỐNG BẮT ĐẦU KHỞI ĐỘNG =====");

    WiFi.mode(WIFI_STA);
    Wire.begin(11,12); 
    
    // --- 1. KHỞI TẠO APP_CONTEXT ĐỂ CHỨA TOÀN BỘ DATA ---
    AppContext_t * act = new AppContext_t();
    act->xMutexSensorData         = xSemaphoreCreateMutex();
    act->xSemaphoreStateChange    = xSemaphoreCreateBinary();
    act->xSemaphoreNeoChange      = xSemaphoreCreateBinary();
    act->xBinarySemaphoreInternet = xSemaphoreCreateBinary();
    
    act->lightStateMux = portMUX_INITIALIZER_UNLOCKED;
    act->controlMode   = CONTROL_MODE_AUTO;
    act->led1State     = false;
    act->led2State     = false;
    
    act->core_iot_server = "mqtt.thingsboard.cloud";
    act->core_iot_port   = "1883";

    act->lcd = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 
    act->dht = new DHT20();
    act->pixels = new Adafruit_NeoPixel(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    // ----------------------------------------------------

    act->lcd->begin();
    if(!act->lcd->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Khởi tạo OLED thất bại!"));
    }
    act->dht->begin();

    act->lcd->clearDisplay();              
    act->lcd->setTextSize(1);              
    act->lcd->setTextColor(SSD1306_WHITE); 
    act->lcd->setCursor(0, 0);             
    act->lcd->print("System Starting");    
    act->lcd->display();

    if (!LittleFS.begin(true)) {
        Serial.println("Lỗi khởi tạo LittleFS!");
    }
    
    xTaskCreate(temp_humi_monitor, "Task_Sensor",   4096, (void*)act, 3, NULL);
    xTaskCreate(led_blinky,        "Task_LED",      2048, (void*)act, 2, NULL);
    xTaskCreate(neo_blinky,        "Task_Neo",      2048, (void*)act, 2, NULL);
    xTaskCreate(tiny_ml_task,      "Task_AI",       8192, (void*)act, 2, NULL);
    xTaskCreate(coreiot_task,      "Task_Coreiot",  8192, (void*)act, 2, NULL);
    xTaskCreate(wifi_task,         "Task_WiFi",     4096, (void*)act, 2, NULL);
    xTaskCreate(Task_Toogle_BOOT,  "Task_Toogle",   2048, (void*)act, 2, NULL);
}

void loop() {
    // Loop nhường quyền cho các Task khác
    vTaskDelay(pdMS_TO_TICKS(100)); 
}