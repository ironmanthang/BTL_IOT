#include "task_handler.h"
#include <Adafruit_NeoPixel.h>

void handleWebSocketMessage(String message) {
    Serial.println("📩 Web nhận: " + message);
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.println("❌ Lỗi parse JSON!");
        return;
    }

    JsonObject value = doc["value"];
    if (doc["page"] == "device") {
        int gpio = value["gpio"];
        String status = value["status"].as<String>();
        bool isOn = status.equalsIgnoreCase("ON");

        if (gpio == 45) {
            // TẠO ĐỐI TƯỢNG CỤC BỘ BẰNG TỪ KHÓA STATIC
            // Biến này bị nhốt hoàn toàn trong hàm này, không ai thấy được nó,
            // đồng thời không dùng malloc nhiều lần gây tràn RAM.
            static Adafruit_NeoPixel webPixels(1, 45, NEO_GRB + NEO_KHZ800);
            webPixels.begin();
            
            if (isOn) {
                webPixels.setPixelColor(0, webPixels.Color(0, 0, 255)); // Bật Xanh Dương
            } else {
                webPixels.setPixelColor(0, webPixels.Color(0, 0, 0));   // Tắt
            }
            webPixels.show();
            Serial.printf("🌈 Đã điều khiển NeoPixel GPIO 45 thành %s\n", status.c_str());
        } 
        else {
            // Logic cho Relay thường giữ nguyên
            pinMode(gpio, OUTPUT);
            digitalWrite(gpio, isOn ? HIGH : LOW);
            Serial.printf("⚙️ GPIO %d -> %s\n", gpio, status.c_str());
        }
    }
    else if (doc["page"] == "setting") {
        String WIFI_SSID = doc["value"]["ssid"].as<String>();
        String WIFI_PASS = doc["value"]["password"].as<String>();
        String CORE_IOT_TOKEN = doc["value"]["token"].as<String>();
        String CORE_IOT_SERVER = doc["value"]["server"].as<String>();
        String CORE_IOT_PORT = doc["value"]["port"].as<String>();

        Serial.println("📥 Nhận cấu hình từ WebSocket:");
        Save_info_File(WIFI_SSID, WIFI_PASS, CORE_IOT_TOKEN, CORE_IOT_SERVER, CORE_IOT_PORT);
    }
}