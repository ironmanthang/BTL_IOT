#include "task_wifi.h" // Nhớ include thư viện <WiFi.h> trong file header này
#include "global.h"

void wifi_task(void *pvParameters) {
    // 1. Nhận hộp tài nguyên
    AppContext_t * act = (AppContext_t *)pvParameters;

    // Thay đổi tên WiFi của bạn ở đây (Hoặc có thể đưa vào AppContext_t ở main.cpp)
    const char* ssid = "Redmi 12";
    const char* password = "12345678";

    // Khởi tạo chế độ Trạm (Station - Kết nối vào Router)
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("[WiFi Task] Đang kết nối đến ");
    Serial.println(ssid);

    bool was_connected = false;

    while(1) {
        // 2. Theo dõi trạng thái mạng liên tục
        if (WiFi.status() == WL_CONNECTED) {
            
            // Nếu vừa mới kết nối thành công (trước đó chưa có mạng)
            if (!was_connected) {
                Serial.println("[WiFi Task] 🟢 ĐÃ KẾT NỐI!");
                Serial.print("[WiFi Task] IP: ");
                Serial.println(WiFi.localIP());

                // Kích hoạt Semaphore báo cho Task CoreIoT biết "ĐÃ CÓ INTERNET!"
                if (act->xBinarySemaphoreInternet != NULL) {
                    xSemaphoreGive(act->xBinarySemaphoreInternet);
                }
                
                was_connected = true;
            }
            
        } else {
            
            // Nếu bị rớt mạng (trước đó đang có mạng)
            if (was_connected) {
                Serial.println("[WiFi Task] 🔴 MẤT KẾT NỐI! Đang thử lại...");
                WiFi.disconnect();
                WiFi.reconnect(); // Lệnh tự động kết nối lại của ESP32
                was_connected = false;
            }
        }

        // Không cần chạy quá nhanh, kiểm tra trạng thái mạng mỗi 2 giây là đẹp
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}