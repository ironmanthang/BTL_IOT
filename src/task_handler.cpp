#include "task_handler.h"
#include <ESPAsyncWebServer.h>
#include "global.h"
#include "led_blinky.h"
#include "neo_blinky.h"

extern AsyncWebSocket ws;

void broadcastControlState(const char *note) {
    bool led1On = false, led2On = false;
    uint8_t r = 0, g = 0, b = 0;

    getLightStates(&led1On, &led2On, &r, &g, &b);

    StaticJsonDocument<256> out;
    out["page"] = "control";
    out["mode"] = (getControlMode() == CONTROL_MODE_MANUAL) ? "MANUAL" : "AUTO";
    out["led1"] = led1On ? "ON" : "OFF";
    out["led2"] = led2On ? "ON" : "OFF";

    JsonObject color = out.createNestedObject("led2_color");
    color["r"] = r; color["g"] = g; color["b"] = b;
    
    if (note != nullptr) out["note"] = note;

    String payload;
    serializeJson(out, payload);
    ws.textAll(payload);
}

void handleWebSocketMessage(String message) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, message)) return;

    JsonObject value = doc["value"];
    String page = doc["page"].as<String>();

    if (page == "control") {
        String mode = value["mode"].as<String>();
        setControlMode(mode.equalsIgnoreCase("MANUAL") ? CONTROL_MODE_MANUAL : CONTROL_MODE_AUTO);
        
        // Kích NeoPixel cập nhật lại màu nếu về AUTO
        if (getControlMode() == CONTROL_MODE_AUTO && xSemaphoreNeoChange != NULL) {
            xSemaphoreGive(xSemaphoreNeoChange);
        }
        broadcastControlState("MODE_UPDATED");
    }
    else if (page == "device") {
        if (getControlMode() != CONTROL_MODE_MANUAL) {
            broadcastControlState("AUTO_LOCKED");
            return;
        }

        int gpio = value["gpio"];
        bool isOn = value["status"].as<String>().equalsIgnoreCase("ON");

        if (gpio == NEO_PIN) {
            // Chỉ cập nhật màu và "bấm chuông" gọi Neo Task
            if (isOn) setLed2Color(0, 0, 255);
            else      setLed2Color(0, 0, 0);

            if (xSemaphoreNeoChange != NULL) {
                xSemaphoreGive(xSemaphoreNeoChange);
            }
        } 
        else {
            pinMode(gpio, OUTPUT);
            digitalWrite(gpio, isOn ? HIGH : LOW);
            if (gpio == LED_GPIO) setLed1State(isOn);
        }

        broadcastControlState("DEVICE_UPDATED");
    }
    else if (page == "setting") {
        Save_info_File(value["ssid"].as<String>(), value["password"].as<String>(), 
                       value["token"].as<String>(), value["server"].as<String>(), 
                       value["port"].as<String>());
    }
}