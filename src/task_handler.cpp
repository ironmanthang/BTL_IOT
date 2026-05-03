#include "task_handler.h"
#include <ESPAsyncWebServer.h>
#include "led_blinky.h"
#include "neo_blinky.h"

extern AsyncWebSocket ws;

void broadcastControlState(AppContext_t *act, const char *note) {
    bool led1On = false, led2On = false;
    uint8_t r = 0, g = 0, b = 0;

    getLightStates(act, &led1On, &led2On, &r, &g, &b);

    StaticJsonDocument<256> out;
    out["page"] = "control";
    out["mode"] = (getControlMode(act) == CONTROL_MODE_MANUAL) ? "MANUAL" : "AUTO";
    out["led1"] = led1On ? "ON" : "OFF";
    out["led2"] = led2On ? "ON" : "OFF";

    JsonObject color = out.createNestedObject("led2_color");
    color["r"] = r; color["g"] = g; color["b"] = b;
    
    if (note != nullptr) out["note"] = note;

    String payload;
    serializeJson(out, payload);
    ws.textAll(payload);
}

void handleWebSocketMessage(String message, AppContext_t *act) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, message)) return;

    JsonObject value = doc["value"];
    String page = doc["page"].as<String>();

    if (page == "control") {
        String mode = value["mode"].as<String>();
        setControlMode(act, mode.equalsIgnoreCase("MANUAL") ? CONTROL_MODE_MANUAL : CONTROL_MODE_AUTO);
        
        if (getControlMode(act) == CONTROL_MODE_AUTO && act->xSemaphoreNeoChange != NULL) {
            xSemaphoreGive(act->xSemaphoreNeoChange);
        }
        broadcastControlState(act, "MODE_UPDATED");
    }
    else if (page == "device") {
        if (getControlMode(act) != CONTROL_MODE_MANUAL) {
            broadcastControlState(act, "AUTO_LOCKED");
            return;
        }

        int gpio = value["gpio"];
        bool isOn = value["status"].as<String>().equalsIgnoreCase("ON");

        if (gpio == NEO_PIN) {
            if (isOn) setLed2Color(act, 0, 0, 255);
            else      setLed2Color(act, 0, 0, 0);

            if (act->xSemaphoreNeoChange != NULL) {
                xSemaphoreGive(act->xSemaphoreNeoChange);
            }
        } 
        else {
            pinMode(gpio, OUTPUT);
            digitalWrite(gpio, isOn ? HIGH : LOW);
            if (gpio == LED_GPIO) setLed1State(act, isOn);
        }

        broadcastControlState(act, "DEVICE_UPDATED");
    }
}