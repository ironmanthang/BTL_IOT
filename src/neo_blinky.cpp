#include "neo_blinky.h"
#include "global.h"

// ĐÃ XÓA EXTERN ADAFRUIT_NEOPIXEL PIXELS Ở ĐÂY! Hoàn toàn độc lập!

void neo_blinky(void *pvParameters) {
    // 1. Nhận cái Hộp
    AppContext_t * act = (AppContext_t *)pvParameters;
    DisplayState_t current_state = STATE_NORMAL;

    // 2. Gọi hàm thông qua act->pixels
    act->pixels->begin();
    act->pixels->clear();
    act->pixels->show(); // Tắt hết LED lúc khởi động

    while(1) {
        if (act->xSemaphoreNeoChange != NULL) {
            if (xSemaphoreTake(act->xSemaphoreNeoChange, portMAX_DELAY) == pdTRUE) {
                
                // Mở Mutex lấy dữ liệu
                if (act->xMutexSensorData != NULL) {
                    if (xSemaphoreTake(act->xMutexSensorData, portMAX_DELAY) == pdTRUE) {
                        current_state = act->sensorData.state;
                        xSemaphoreGive(act->xMutexSensorData); 
                    }
                }

                // Đổi màu đèn. Lưu ý cách gọi hàm Color cũng dùng qua act->pixels
                if (current_state == STATE_NORMAL) {
                    act->pixels->setPixelColor(0, act->pixels->Color(0, 255, 0)); 
                } else if (current_state == STATE_WARNING) {
                    act->pixels->setPixelColor(0, act->pixels->Color(255, 255, 0)); 
                } else if (current_state == STATE_CRITICAL) {
                    act->pixels->setPixelColor(0, act->pixels->Color(255, 0, 0)); 
                }
                
                act->pixels->show(); 
            }
        }
    }
}