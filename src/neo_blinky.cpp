#include "neo_blinky.h"
#include "global.h"

void neo_blinky(void *pvParameters) {
    AppContext_t * act = (AppContext_t *)pvParameters;
    DisplayState_t current_state = STATE_NORMAL;

    act->pixels->begin();
    act->pixels->clear();
    act->pixels->show(); 
    setLed2Color(0, 0, 0);

    while(1) {
        // Đợi lệnh (từ Web/Cloud) hoặc hết hạn 200ms (để chạy AUTO)
        if (act->xSemaphoreNeoChange != NULL) {
            xSemaphoreTake(act->xSemaphoreNeoChange, pdMS_TO_TICKS(200));
        } else {
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        // Ưu tiên 1: Chế độ bằng tay
        if (getControlMode() == CONTROL_MODE_MANUAL) {
            uint8_t r, g, b;
            getLightStates(NULL, NULL, &r, &g, &b); 
            
            act->pixels->setPixelColor(0, act->pixels->Color(r, g, b));
            act->pixels->show();
            continue; // Bỏ qua đoạn code AUTO bên dưới
        }

        // Ưu tiên 2: Chế độ tự động
        if (act->xMutexSensorData != NULL) {
            if (xSemaphoreTake(act->xMutexSensorData, portMAX_DELAY) == pdTRUE) {
                current_state = act->sensorData.state;
                xSemaphoreGive(act->xMutexSensorData);
            }
        }

        if (current_state == STATE_NORMAL) {
            act->pixels->setPixelColor(0, act->pixels->Color(0, 255, 0));
            setLed2Color(0, 255, 0);
        } else if (current_state == STATE_WARNING) {
            act->pixels->setPixelColor(0, act->pixels->Color(255, 255, 0));
            setLed2Color(255, 255, 0);
        } else if (current_state == STATE_CRITICAL) {
            act->pixels->setPixelColor(0, act->pixels->Color(255, 0, 0));
            setLed2Color(255, 0, 0);
        }
        
        act->pixels->show();
    }
}