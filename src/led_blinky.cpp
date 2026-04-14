#include "led_blinky.h"

void led_blinky(void *pvParameters) {
    pinMode(LED_GPIO, OUTPUT);
    DisplayState_t current_state = STATE_NORMAL;
    SensorData_t data;

    while(1) {
        if (xSemaphoreTake(xSemaphoreStateChange, 0) == pdTRUE) {
            if (sensorData_read(&data)) {
                current_state = data.state;
            }
        }

        if (current_state == STATE_NORMAL) {
            digitalWrite(LED_GPIO, LOW);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if (current_state == STATE_WARNING) {
            digitalWrite(LED_GPIO, !digitalRead(LED_GPIO));
            vTaskDelay(pdMS_TO_TICKS(1000)); 
        } else if (current_state == STATE_CRITICAL) {
            digitalWrite(LED_GPIO, !digitalRead(LED_GPIO));
            vTaskDelay(pdMS_TO_TICKS(200)); 
        }
    }
}