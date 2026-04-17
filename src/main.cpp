#include <Arduino.h>
#include <Wire.h>
#include "global.h"
#include <Adafruit_NeoPixel.h>
#include "temp_humi_monitor.h"
#include "led_blinky.h"
#include "neo_blinky.h"
#include "tinyml.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
// DHT20 dht;
// Adafruit_NeoPixel pixels(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_SSD1306 lcd(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT20 dht;
Adafruit_NeoPixel pixels(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    Serial.begin(115200);

    Wire.begin(11,12); 
    lcd.begin();
    // 1. KHỞI TẠO OLED CHUẨN
    if(!lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Khởi tạo OLED thất bại!"));
    }
    dht.begin();

    lcd.clearDisplay();              // Bước 1: Xóa sạch bộ đệm
    lcd.setTextSize(1);              // Bước 2: Chọn cỡ chữ
    lcd.setTextColor(SSD1306_WHITE); // Bước 3: Chọn màu chữ (Màu trắng)
    lcd.setCursor(0, 0);             // Bước 4: Đặt tọa độ
    lcd.print("System Starting");    // Bước 5: Viết chữ vào bộ đệm
    
    lcd.display();

    xMutexSensorData      = xSemaphoreCreateMutex();
    xSemaphoreStateChange = xSemaphoreCreateBinary();
    xSemaphoreNeoChange   = xSemaphoreCreateBinary();
    // xBinarySemaphoreInternet = xSemaphoreCreateBinary();
    
    // 4. Chạy Task
    xTaskCreate(temp_humi_monitor, "Task_Sensor", 4096, NULL, 3, NULL);
    xTaskCreate(led_blinky,        "Task_LED",    2048, NULL, 2, NULL);
    xTaskCreate(neo_blinky,        "Task_Neo",    2048, NULL, 2, NULL);
    xTaskCreate(tiny_ml_task,      "Task_AI",     8192, NULL, 2, NULL);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}