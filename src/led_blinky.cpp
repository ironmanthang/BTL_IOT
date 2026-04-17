#include "led_blinky.h"

void led_blinky(void *pvParameters) {
    pinMode(LED_GPIO, OUTPUT);
    DisplayState_t current_state = STATE_NORMAL;
    
    // 1. Nhận cái Hộp từ tham số
    AppContext_t * act = (AppContext_t *)pvParameters;

    while(1) {
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