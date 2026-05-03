#include "task_webserver.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool webserver_isrunning = false;
static AppContext_t *web_act = nullptr; // Chỉ tồn tại nội bộ trong file này

void Webserver_sendata(String data) {
    if (ws.count() > 0) ws.textAll(data);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT) {
            String message = String((char *)data).substring(0, len);
            handleWebSocketMessage(message, web_act); // Truyền Hộp dữ liệu sang Handler
        }
    }
}

void connnectWSV(AppContext_t *act) {
    web_act = act; 
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(LittleFS, "/index.html", "text/html"); });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(LittleFS, "/script.js", "application/javascript"); });
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(LittleFS, "/styles.css", "text/css"); });

    // Dùng kỹ thuật Capture Lambda [act] để hàm vô danh sử dụng được biến
    server.on("/save_wifi", HTTP_GET, [act](AsyncWebServerRequest *request){
        String ssid = request->hasParam("ssid") ? request->getParam("ssid")->value() : "";
        String pass = request->hasParam("pass") ? request->getParam("pass")->value() : "";
        String srv  = request->hasParam("server") ? request->getParam("server")->value() : "";
        String tkn  = request->hasParam("token") ? request->getParam("token")->value() : "";
        String port = request->hasParam("port") ? request->getParam("port")->value() : "1883";
        
        request->send(200, "text/plain", "OK");
        Save_info_File(ssid, pass, tkn, srv, port, act); 
    });

    server.begin();
    ElegantOTA.begin(&server);  
    webserver_isrunning = true;
}

unsigned long last_broadcast = 0;

void Webserver_reconnect(AppContext_t *act) {
    if (!webserver_isrunning) {
        connnectWSV(act);
    }
    ElegantOTA.loop();  

    if (millis() - last_broadcast > 3000) {
        last_broadcast = millis();
        if (webserver_isrunning && ws.count() > 0) {
            SensorData_t data;
            bool hasSensor = sensorData_read(act, &data);

            bool led1On = false, led2On = false;
            uint8_t r = 0, g = 0, b = 0;
            getLightStates(act, &led1On, &led2On, &r, &g, &b);

            DynamicJsonDocument doc(1024);
            if (hasSensor) {
                doc["temp"] = data.temperature;
                doc["hum"]  = data.humidity;
                doc["ai"]   = data.ai_score;
            }

            JsonObject control = doc.createNestedObject("control");
            control["mode"] = (getControlMode(act) == CONTROL_MODE_MANUAL) ? "MANUAL" : "AUTO";

            JsonObject led1 = control.createNestedObject("led1");
            led1["gpio"] = 48; // Bỏ chữ LED_GPIO nếu ko import
            led1["status"] = led1On ? "ON" : "OFF";

            JsonObject led2 = control.createNestedObject("led2");
            led2["gpio"] = 45; // Bỏ chữ NEO_PIN nếu ko import
            led2["status"] = led2On ? "ON" : "OFF";
            JsonObject color = led2.createNestedObject("color");
            color["r"] = r; color["g"] = g; color["b"] = b;

            JsonObject sys = doc.createNestedObject("system");
            
            // 1. Thông tin bộ nhớ RAM
            sys["FreeHeap"] = String(ESP.getFreeHeap() / 1024.0, 1) + " KB";
            sys["TotalHeap"] = String(ESP.getHeapSize() / 1024.0, 1) + " KB";
            
            // 2. Thông tin mạng
            sys["WifiSignal"] = String(WiFi.RSSI()) + " dBm";
            sys["IpAddress"] = WiFi.localIP().toString();
            sys["MacAddress"] = WiFi.macAddress();

            // 3. Thông tin phần cứng vi điều khiển
            String chipModel = ESP.getChipModel();
            sys["ChipModel"] = chipModel + " (Rev " + String(ESP.getChipRevision()) + ")";
            sys["CpuSpeed"] = String(ESP.getCpuFreqMHz()) + " MHz";

            // 4. Thời gian đã chạy (Uptime) tính từ lúc cấp nguồn
            unsigned long uptimeSec = millis() / 1000;
            unsigned long h = uptimeSec / 3600;
            unsigned long m = (uptimeSec % 3600) / 60;
            unsigned long s = uptimeSec % 60;
            char uptimeStr[20];
            sprintf(uptimeStr, "%02luh %02lum %02lus", h, m, s);
            sys["Uptime"] = String(uptimeStr);

            String payload;
            serializeJson(doc, payload);
            Webserver_sendata(payload);
        }
    }
}