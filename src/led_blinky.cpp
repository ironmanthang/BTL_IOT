#include "led_blinky.h"

void led_blinky(void *pvParameters) {
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LOW);
    setLed1State(false);
    DisplayState_t current_state = STATE_NORMAL;
    
    // 1. Nhận cái Hộp từ tham số
    AppContext_t * act = (AppContext_t *)pvParameters;

    while(1) {
        if (getControlMode() == CONTROL_MODE_MANUAL) {
            vTaskDelay(pdMS_TO_TICKS(80));
            continue;
        }

        if (act->xSemaphoreStateChange != NULL) {
            if (xSemaphoreTake(act->xSemaphoreStateChange, 0) == pdTRUE) {
                
                if (act->xMutexSensorData != NULL) {
                    if (xSemaphoreTake(act->xMutexSensorData, portMAX_DELAY) == pdTRUE) {
                        
                        current_state = act->sensorData.state;
                        
                        xSemaphoreGive(act->xMutexSensorData);
                    }
                }
            }
        }

        if (current_state == STATE_NORMAL) {
            digitalWrite(LED_GPIO, LOW);
            setLed1State(false);
            vTaskDelay(pdMS_TO_TICKS(500)); 
        } else if (current_state == STATE_WARNING) {
            bool next = !digitalRead(LED_GPIO);
            digitalWrite(LED_GPIO, next ? HIGH : LOW);
            setLed1State(next);
            vTaskDelay(pdMS_TO_TICKS(1000)); 
        } else if (current_state == STATE_CRITICAL) {
            bool next = !digitalRead(LED_GPIO);
            digitalWrite(LED_GPIO, next ? HIGH : LOW);
            setLed1State(next);
            vTaskDelay(pdMS_TO_TICKS(200)); 
        }
    }
}