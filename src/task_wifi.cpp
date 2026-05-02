#include "task_wifi.h"
#include "global.h"
#include "task_check_info.h"

void wifi_task(void *pvParameters) {
    AppContext_t * act = (AppContext_t *)pvParameters;

    // 1. Đọc cấu hình từ LittleFS (Đã lưu từ Web trước đó)
    Load_info_File();

    // 2. Bật chế độ vừa phát (AP) vừa nhận (STA)
    WiFi.mode(WIFI_AP_STA);

    // 3. Nếu đã có cấu hình, thử kết nối
    if (WIFI_SSID.length() > 0) {
        Serial.print("[WiFi] Đang kết nối mạng: ");
        Serial.println(WIFI_SSID);
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());

        int timeout = 0;
        // Đợi tối đa 10 giây
        while (WiFi.status() != WL_CONNECTED && timeout < 20) {
            vTaskDelay(pdMS_TO_TICKS(500));
            Serial.print(".");
            timeout++;
        }
    }

    // 4. Nếu không kết nối được (hoặc chưa cài đặt), bật AP
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[WiFi] 🔴 Chuyển sang phát WiFi cấu hình (AP Mode)");
        WiFi.softAP("YOLO_UNO_Config", "12345678"); 
        Serial.print("[WiFi] Truy cập IP Web: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("\n[WiFi] 🟢 KẾT NỐI THÀNH CÔNG!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        
        // Mở khóa cho CoreIoT chạy
        if (act->xBinarySemaphoreInternet != NULL) {
            xSemaphoreGive(act->xBinarySemaphoreInternet);
        }
    }

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}