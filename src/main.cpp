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
#include <LittleFS.h> // BẮT BUỘC PHẢI CÓ ĐỂ CHẠY WEB

void setup() {
    Serial.begin(115200);
    // Dừng 3 giây để Serial Monitor kịp bật lên
    delay(3000); 
    Serial.println("\n\n===== HỆ THỐNG BẮT ĐẦU KHỞI ĐỘNG =====");

    // Kích hoạt lõi mạng trước để tránh lỗi Invalid mbox
    WiFi.mode(WIFI_STA);

    Wire.begin(11,12); 
    
    DHT20 * dht = new DHT20();
    Adafruit_SSD1306 * lcd = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 
    AppContext_t * act = new AppContext_t();
    Adafruit_NeoPixel * myPixels = new Adafruit_NeoPixel(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

    act -> xMutexSensorData      = xSemaphoreCreateMutex();

    // =================================================================
    // BỔ SUNG CẦU NỐI MUTEX: Cho phép Webserver đọc được dữ liệu cảm biến
    extern SemaphoreHandle_t xMutexSensorData;
    xMutexSensorData = act->xMutexSensorData;
    // =================================================================

    act -> xSemaphoreStateChange = xSemaphoreCreateBinary();
    act -> xSemaphoreNeoChange   = xSemaphoreCreateBinary();
    act -> xBinarySemaphoreInternet = xSemaphoreCreateBinary();
    act -> lcd = lcd;
    act -> dht = dht;
    act->pixels = myPixels;

    act -> lcd->begin();
    if(!act -> lcd->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Khởi tạo OLED thất bại!"));
    }
    act -> dht -> begin();

    act -> lcd -> clearDisplay();              
    act -> lcd -> setTextSize(1);              
    act -> lcd -> setTextColor(SSD1306_WHITE); 
    act -> lcd -> setCursor(0, 0);             
    act -> lcd -> print("System Starting");    
    act -> lcd -> display();

    // KHỞI TẠO LITTLEFS ĐỂ ĐỌC GIAO DIỆN WEB
    if (!LittleFS.begin(true)) {
        Serial.println("❌ Lỗi khởi tạo LittleFS!");
    } else {
        Serial.println("✅ LittleFS đã sẵn sàng.");
    }
    
    // Chạy các Task
    xTaskCreate(temp_humi_monitor, "Task_Sensor",   4096, (void*)act, 3, NULL);
    xTaskCreate(led_blinky,        "Task_LED",      2048, (void*)act, 2, NULL);
    xTaskCreate(neo_blinky,        "Task_Neo",      2048, (void*)act, 2, NULL);
    xTaskCreate(tiny_ml_task,      "Task_AI",       8192, (void*)act, 2, NULL);
    xTaskCreate(coreiot_task,      "Task_Coreiot",  10240, (void*)act, 2, NULL);
    xTaskCreate(wifi_task,         "Task_WiFi",     4096, (void*)act, 2, NULL);
    xTaskCreate(Task_Toogle_BOOT,  "Task_Toogle",   4096, (void*)act, 2, NULL);
}

void loop() {
    // CHỈ BẬT WEB SERVER KHI ĐÃ CÓ KẾT NỐI WIFI
    Webserver_reconnect();
    
    // Nghỉ 10ms để nhường CPU cho các task khác, giúp vòng lặp chạy liên tục
    vTaskDelay(pdMS_TO_TICKS(10)); 
}