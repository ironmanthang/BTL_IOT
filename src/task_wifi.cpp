#include "task_wifi.h" // Nhớ include thư viện <WiFi.h> trong file header này
#include "global.h"

void wifi_task(void *pvParameters) {
    // 1. Nhận hộp tài nguyên
    AppContext_t * act = (AppContext_t *)pvParameters;

    // Thay đổi tên WiFi của bạn ở đây (Hoặc có thể đưa vào AppContext_t ở main.cpp)
    const char* ssid = "Hoang An";
    const char* password = "hoangan27012016@";

    // Khởi tạo chế độ Trạm (Station - Kết nối vào Router)
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("[WiFi Task] Đang kết nối đến ");
    Serial.println(ssid);

    bool was_connected = false;

    while(1) {
        // CỔNG BẢO VỆ: Phải có WiFi và IP phải hợp lệ thì mới được chạy vào trong
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP().toString() != "0.0.0.0") {
            
            if (!was_connected) {
                // In khung viền cho dễ nhìn
                Serial.println("\n=======================================");
                Serial.println("[WiFi Task] 🟢 ĐÃ CÓ INTERNET THỰC SỰ!");
                Serial.print("[WiFi Task] 🌐 ĐỊA CHỈ IP LÀ: ");
                Serial.println(WiFi.localIP());
                Serial.println("=======================================\n");

                // Ép máy tính in chữ ra màn hình ngay lập tức và bắt mạch "ngủ" 2 giây
                Serial.flush();
                vTaskDelay(pdMS_TO_TICKS(2000)); 

                // Sau khi ngủ 2 giây xong mới mở khóa cho Task MQTT và AI chạy
                if (act->xBinarySemaphoreInternet != NULL) {
                    xSemaphoreGive(act->xBinarySemaphoreInternet);
                }
                was_connected = true;
            }

        } 
        // NẾU RỚT MẠNG THÌ XỬ LÝ Ở ĐÂY
        else {
            if (was_connected) {
                Serial.println("[WiFi Task] 🔴 MẤT IP/MẤT KẾT NỐI!");
                was_connected = false;
            }
        }

        // Vòng lặp nghỉ 2 giây rồi kiểm tra WiFi lại một lần
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}