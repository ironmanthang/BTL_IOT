#include "task_wifi.h" // Nhớ include thư viện <WiFi.h> trong file header này
#include "global.h"

void wifi_task(void *pvParameters) {
    // 1. Nhận hộp tài nguyên
    AppContext_t * act = (AppContext_t *)pvParameters;

    // Thay đổi tên WiFi của bạn ở đây (Hoặc có thể đưa vào AppContext_t ở main.cpp)
    const char* ssid = "abcd";
    const char* password = "12345678";

    // Khởi tạo chế độ Trạm (Station - Kết nối vào Router)
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("[WiFi Task] Đang kết nối đến ");
    Serial.println(ssid);

    bool was_connected = false;

    while(1) {
        // 2. Kiểm tra trạng thái: Phải CÓ KẾT NỐI và IP phải KHÁC 0.0.0.0
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP().toString() != "0.0.0.0") {
            
            if (!was_connected) {
                Serial.println("[WiFi Task] 🟢 ĐÃ CÓ INTERNET THỰC SỰ!");
                Serial.print("[WiFi Task] IP Cấp bởi điện thoại: ");
                Serial.println(WiFi.localIP());

                // Chỉ khi có IP rồi mới mở khóa cho Task MQTT chạy
                if (act->xBinarySemaphoreInternet != NULL) {
                    xSemaphoreGive(act->xBinarySemaphoreInternet);
                }
                
                was_connected = true;
            }
            
        } else {
            if (was_connected) {
                Serial.println("[WiFi Task] 🔴 MẤT IP/MẤT KẾT NỐI!");
                was_connected = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}