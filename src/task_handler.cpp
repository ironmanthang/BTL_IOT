#include "task_handler.h"
#include <Adafruit_NeoPixel.h>
#include <ESPAsyncWebServer.h>
#include "global.h"
#include "led_blinky.h"
#include "neo_blinky.h"

extern AsyncWebSocket ws;

static void broadcastControlState(const char *note = nullptr) {
    bool led1On = false;
    bool led2On = false;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    getLightStates(&led1On, &led2On, &r, &g, &b);

    StaticJsonDocument<256> out;
    out["page"] = "control";
    out["mode"] = (getControlMode() == CONTROL_MODE_MANUAL) ? "MANUAL" : "AUTO";
    out["led1"] = led1On ? "ON" : "OFF";
    out["led2"] = led2On ? "ON" : "OFF";

    JsonObject color = out.createNestedObject("led2_color");
    color["r"] = r;
    color["g"] = g;
    color["b"] = b;
    if (note != nullptr) {
        out["note"] = note;
    }

    String payload;
    serializeJson(out, payload);
    ws.textAll(payload);
}

void handleWebSocketMessage(String message) {
    Serial.println("📩 Web nhận: " + message);
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.println("❌ Lỗi parse JSON!");
        return;
    }

    JsonObject value = doc["value"];
    if (doc["page"] == "control") {
        String mode = value["mode"].as<String>();
        if (mode.equalsIgnoreCase("MANUAL")) {
            setControlMode(CONTROL_MODE_MANUAL);
            Serial.println("🔧 Chuyển sang chế độ MANUAL");
        } else {
            setControlMode(CONTROL_MODE_AUTO);
            Serial.println("🤖 Chuyển sang chế độ AUTO");
        }
        broadcastControlState("MODE_UPDATED");
        return;
    }

    if (doc["page"] == "device") {
        if (getControlMode() != CONTROL_MODE_MANUAL) {
            Serial.println("⛔ Bỏ qua lệnh thiết bị vì đang ở AUTO");
            broadcastControlState("AUTO_LOCKED");
            return;
        }

        int gpio = value["gpio"];
        String status = value["status"].as<String>();
        bool isOn = status.equalsIgnoreCase("ON");

        if (gpio == NEO_PIN) {
            // TẠO ĐỐI TƯỢNG CỤC BỘ BẰNG TỪ KHÓA STATIC
            // Biến này bị nhốt hoàn toàn trong hàm này, không ai thấy được nó,
            // đồng thời không dùng malloc nhiều lần gây tràn RAM.
            static Adafruit_NeoPixel webPixels(1, NEO_PIN, NEO_GRB + NEO_KHZ800);
            webPixels.begin();
            
            if (isOn) {
                webPixels.setPixelColor(0, webPixels.Color(0, 0, 255)); // Bật Xanh Dương
                setLed2Color(0, 0, 255);
            } else {
                webPixels.setPixelColor(0, webPixels.Color(0, 0, 0));   // Tắt
                setLed2Color(0, 0, 0);
            }
            webPixels.show();
            Serial.printf("🌈 Đã điều khiển NeoPixel GPIO %d thành %s\n", NEO_PIN, status.c_str());
        } 
        else {
            // Logic cho Relay thường giữ nguyên
            pinMode(gpio, OUTPUT);
            digitalWrite(gpio, isOn ? HIGH : LOW);
            if (gpio == LED_GPIO) {
                setLed1State(isOn);
            }
            Serial.printf("⚙️ GPIO %d -> %s\n", gpio, status.c_str());
        }

        broadcastControlState("DEVICE_UPDATED");
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