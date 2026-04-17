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



void setup() {
    Serial.begin(115200);
    Wire.begin(11,12); 
    
    DHT20 * dht = new DHT20();
    Adafruit_SSD1306 * lcd = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 
    AppContext_t * act = new AppContext_t();
    Adafruit_NeoPixel * myPixels = new Adafruit_NeoPixel(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

    act -> xMutexSensorData      = xSemaphoreCreateMutex();
    act -> xSemaphoreStateChange = xSemaphoreCreateBinary();
    act -> xSemaphoreNeoChange   = xSemaphoreCreateBinary();
    act->  xBinarySemaphoreInternet = xSemaphoreCreateBinary();
    act -> lcd = lcd;
    act -> dht = dht;
    act->pixels = myPixels;

    act -> lcd->begin();
    // 1. KHỞI TẠO OLED CHUẨN
    if(!act -> lcd->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Khởi tạo OLED thất bại!"));
    }
    act -> dht -> begin();

    act -> lcd -> clearDisplay();              // Bước 1: Xóa sạch bộ đệm
    act -> lcd -> setTextSize(1);              // Bước 2: Chọn cỡ chữ
    act -> lcd -> setTextColor(SSD1306_WHITE); // Bước 3: Chọn màu chữ (Màu trắng)
    act -> lcd -> setCursor(0, 0);             // Bước 4: Đặt tọa độ
    act -> lcd -> print("System Starting");    // Bước 5: Viết chữ vào bộ đệm
    
    act -> lcd -> display();



    // xBinarySemaphoreInternet = xSemaphoreCreateBinary();
    
    // 4. Chạy Task
    xTaskCreate(temp_humi_monitor, "Task_Sensor",   4096, (void*)act, 3, NULL);
    xTaskCreate(led_blinky,        "Task_LED",      2048, (void*)act, 2, NULL);
    xTaskCreate(neo_blinky,        "Task_Neo",      2048, (void*)act, 2, NULL);
    xTaskCreate(tiny_ml_task,      "Task_AI",       8192, (void*)act, 2, NULL);
    xTaskCreate(coreiot_task,      "Task_Coreiot",  10240, (void*)act, 2, NULL);
    xTaskCreate(wifi_task,         "Task_WiFi",     4096, (void*)act, 4, NULL);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}