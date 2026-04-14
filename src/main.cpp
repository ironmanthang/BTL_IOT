#include <Arduino.h>
#include <Wire.h>
#include "global.h"
#include <Adafruit_NeoPixel.h>
#include "temp_humi_monitor.h"
#include "led_blinky.h"
#include "neo_blinky.h"

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
DHT20 dht;
Adafruit_NeoPixel pixels(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
void setup() {
    Serial.begin(115200);

    Wire.begin(); 
    lcd.begin();
    lcd.backlight();
    dht.begin();

    lcd.setCursor(0, 0);
    lcd.print("System Starting");

    xMutexSensorData      = xSemaphoreCreateMutex();
    xSemaphoreStateChange = xSemaphoreCreateBinary();
    xSemaphoreNeoChange   = xSemaphoreCreateBinary();
    
    // 4. Chạy Task
    xTaskCreate(temp_humi_monitor, "Task_Sensor", 4096, NULL, 3, NULL);
    xTaskCreate(led_blinky,        "Task_LED",    2048, NULL, 2, NULL);
    xTaskCreate(neo_blinky,        "Task_Neo",    2048, NULL, 2, NULL);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}