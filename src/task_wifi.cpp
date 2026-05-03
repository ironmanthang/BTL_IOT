#include "task_wifi.h"

void wifi_task(void *pvParameters) {
    AppContext_t * act = (AppContext_t *)pvParameters;

    Load_info_File(act);
    WiFi.mode(WIFI_AP_STA);

    bool connected_to_router = false;

    // Nếu có cấu hình, cố gắng kết nối vào WiFi nhà
    if (act->wifi_ssid.length() > 0) {
        Serial.print("\n[WiFi] Đang ket noi vao mang: ");
        Serial.println(act->wifi_ssid);

        WiFi.begin(act->wifi_ssid.c_str(), act->wifi_pass.c_str());
        int timeout = 0;
        
        // Đợi tối đa 10 giây (20 * 500ms)
        while (WiFi.status() != WL_CONNECTED && timeout < 20) {
            vTaskDelay(pdMS_TO_TICKS(500));
            Serial.print(".");
            timeout++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            connected_to_router = true;
        }
    }

    // Kiểm tra kết quả
    if (!connected_to_router) {
        Serial.println("\n\n[WiFi] 🔴 Khong co ket noi hoac mang sai. Dang phat WiFi cau hinh (AP Mode)...");
        WiFi.softAP("YOLO_UNO_Config", "12345678"); 
        Serial.println("==================================================");
        Serial.println("   [WiFi] Phat thanh cong!");
        Serial.println("   [WiFi] SSID (Ten WiFi): YOLO_UNO_Config");
        Serial.println("   [WiFi] Pass (Mat khau): 12345678");
        Serial.print("   [Web]  TRUY CAP TRANG CHU TAI: http://");
        Serial.println(WiFi.softAPIP()); 
        Serial.println("==================================================\n");
    } else {
        Serial.println("\n\n[WiFi] 🟢 KET NOI THANH CONG!");
        Serial.println("==================================================");
        Serial.print("   [WiFi] Mang dang dung: ");
        Serial.println(act->wifi_ssid);
        Serial.print("   [Web]  TRUY CAP TRANG CHU TAI: http://");
        Serial.println(WiFi.localIP());
        Serial.println("==================================================\n");

        if (act->xBinarySemaphoreInternet != NULL) {
            xSemaphoreGive(act->xBinarySemaphoreInternet);
        }
    }

    // Bật Webserver và chạy ngầm
    while(1) {
        Webserver_reconnect(act);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}