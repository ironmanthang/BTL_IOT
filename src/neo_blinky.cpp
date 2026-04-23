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
    setLed2Color(0, 0, 0);

    while(1) {
        if (getControlMode() == CONTROL_MODE_MANUAL) {
            vTaskDelay(pdMS_TO_TICKS(80));
            continue;
        }

        if (act->xSemaphoreNeoChange != NULL) {
            xSemaphoreTake(act->xSemaphoreNeoChange, pdMS_TO_TICKS(200));
        } else {
            vTaskDelay(pdMS_TO_TICKS(200));
        }

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