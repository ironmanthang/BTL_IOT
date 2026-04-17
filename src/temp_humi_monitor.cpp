#include "temp_humi_monitor.h"

extern DHT20 dht; 
// extern LiquidCrystal_I2C lcd;

void temp_humi_monitor(void *pvParameters) {
    DisplayState_t last_state = STATE_NORMAL;

    while(1) {
        dht.read();
        float temp = dht.getTemperature();
        float humi = dht.getHumidity();

        DisplayState_t current_state = STATE_NORMAL;
        if (temp >= TEMP_CRITICAL_THRESHOLD || humi >= HUMI_CRITICAL_THRESHOLD) {
            current_state = STATE_CRITICAL;
        } else if (temp >= TEMP_WARNING_THRESHOLD || humi >= HUMI_WARNING_THRESHOLD) {
            current_state = STATE_WARNING;
        }

        sensorData_write(temp, humi, current_state);

        if (current_state != last_state) {
            if(xSemaphoreStateChange != NULL) xSemaphoreGive(xSemaphoreStateChange);
            if(xSemaphoreNeoChange != NULL)   xSemaphoreGive(xSemaphoreNeoChange);
            last_state = current_state;
        }

        // // Cập nhật LCD
        // lcd.setCursor(0, 0);
        // lcd.printf("T:%.1fC %s ", temp, current_state == STATE_CRITICAL ? "CRIT" : (current_state == STATE_WARNING ? "WARN" : "NORM"));
        // lcd.setCursor(0, 1);
        // lcd.printf("H:%.1f%%   ", humi);

        Serial.print("nhiet do :");
        Serial.print(temp);
        Serial.println(" C");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}