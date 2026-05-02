#include "task_toogle_boot.h"
#include "task_check_info.h"
#include "global.h"

#define BOOT_PIN 0 

void Task_Toogle_BOOT(void *pvParameters) {
    // 1. Nhận cái hộp tài nguyên[cite: 1, 15]
    AppContext_t * act = (AppContext_t *)pvParameters;
    
    unsigned long buttonPressStartTime = 0;
    pinMode(BOOT_PIN, INPUT_PULLUP); 

    while (true) {
        if (digitalRead(BOOT_PIN) == LOW) {
            if (buttonPressStartTime == 0) {
                buttonPressStartTime = millis(); 
            } 
            else if (millis() - buttonPressStartTime > 2000) {
                Serial.println("🔄 Đang xóa cấu hình...");
                
                // Bạn có thể dùng act ở đây để thông báo lên LCD nếu muốn:
                // act->lcd->clearDisplay();
                // act->lcd->print("Reset WiFi...");
                // act->lcd->display();
                
                Delete_info_File(); // Xóa cấu hình và tự restart[cite: 8, 13]
            }
        } else {
            buttonPressStartTime = 0; 
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}