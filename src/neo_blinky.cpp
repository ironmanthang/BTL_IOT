#include "neo_blinky.h"
#include "global.h"

void neo_blinky(void *pvParameters) {
    AppContext_t * act = (AppContext_t *)pvParameters;
    DisplayState_t current_state = STATE_NORMAL;

    act->pixels->begin();
    act->pixels->clear();
    act->pixels->show(); 
    
    // Thêm 'act' vào tham số đầu tiên
    setLed2Color(act, 0, 0, 0);

    while(1) {
        if (act->xSemaphoreNeoChange != NULL) {
            xSemaphoreTake(act->xSemaphoreNeoChange, pdMS_TO_TICKS(200));
        } else {
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        // Thêm 'act' vào hàm getControlMode
        if (getControlMode(act) == CONTROL_MODE_MANUAL) {
            uint8_t r, g, b;
            
            // Thêm 'act' vào hàm getLightStates
            getLightStates(act, NULL, NULL, &r, &g, &b); 
            
            act->pixels->setPixelColor(0, act->pixels->Color(r, g, b));
            act->pixels->show();
            continue; 
        }

        if (act->xMutexSensorData != NULL) {
            if (xSemaphoreTake(act->xMutexSensorData, portMAX_DELAY) == pdTRUE) {
                current_state = act->sensorData.state;
                xSemaphoreGive(act->xMutexSensorData);
            }
        }

        if (current_state == STATE_NORMAL) {
            act->pixels->setPixelColor(0, act->pixels->Color(0, 255, 0));
            // Thêm 'act'
            setLed2Color(act, 0, 255, 0);
        } else if (current_state == STATE_WARNING) {
            act->pixels->setPixelColor(0, act->pixels->Color(255, 255, 0));
            // Thêm 'act'
            setLed2Color(act, 255, 255, 0);
        } else if (current_state == STATE_CRITICAL) {
            act->pixels->setPixelColor(0, act->pixels->Color(255, 0, 0));
            // Thêm 'act'
            setLed2Color(act, 255, 0, 0);
        }
        
        act->pixels->show();
    }
}