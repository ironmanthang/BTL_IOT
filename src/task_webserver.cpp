#include "task_webserver.h"
#include "led_blinky.h"
#include "neo_blinky.h"

// ---------------------------------------------------------------------------
// Task 4 — Web Server in Access Point Mode
//
// Architecture:
//   - ESPAsyncWebServer on port 80 (non-blocking, runs on its own internal task)
//   - AsyncWebSocket at endpoint "/ws" for real-time bidirectional communication
//   - LittleFS serves static frontend files (index.html, script.js, styles.css)
//   - ElegantOTA enables over-the-air firmware updates via HTTP
//
// Sensor data flow (Task 3 integration):
//   temp_humi_monitor  →  sensorData_write()  →  [mutex]  →  sharedSensorData
//   Webserver_reconnect  →  sensorData_read()  →  JSON  →  WebSocket broadcast
//
// WebSocket JSON protocol:
//   Server → Client (push, every 3s): {"temp": 26.8, "hum": 55.5}
//   Client → Server (command):        {"page":"device", "led1":"ON", ...}
// ---------------------------------------------------------------------------

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Tracks whether the HTTP server has been started at least once.
// Used by Webserver_reconnect() to avoid re-initializing on every loop() call.
bool webserver_isrunning = false;

// ---------------------------------------------------------------------------
// Webserver_sendata — Broadcast a text payload to ALL currently connected
// WebSocket clients. Silently skips if no clients are connected.
// Called once per broadcast cycle with the JSON sensor payload.
// ---------------------------------------------------------------------------
void Webserver_sendata(String data)
{
    if (ws.count() > 0)
    {
        ws.textAll(data); // Gửi đến tất cả client đang kết nối
    }
    else
    {
        Serial.println("⚠️ Không có client WebSocket nào đang kết nối!");
    }
}

// ---------------------------------------------------------------------------
// onEvent — WebSocket event callback registered with the AsyncWebSocket handler.
//
// Handles three event types:
//   WS_EVT_CONNECT    — logs new client connection (id + remote IP)
//   WS_EVT_DISCONNECT — logs client disconnection
//   WS_EVT_DATA       — receives text frame and forwards to handleWebSocketMessage()
//                       for JSON command parsing (device control, settings, etc.)
// ---------------------------------------------------------------------------
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;

        if (info->opcode == WS_TEXT)
        {
            // Extract the text payload from the raw byte buffer
            String message;
            message += String((char *)data).substring(0, len);
            // Route the command to the appropriate handler (GPIO control, Wi-Fi settings, etc.)
            handleWebSocketMessage(message);
        }
    }
}

// ---------------------------------------------------------------------------
// connnectWSV — Initialize and start the AsyncWebServer.
// Called once when the server needs to be started (or restarted after stop).
//
// Registers:
//   - WebSocket handler at "/ws"
//   - Static file routes: "/" → index.html, "/script.js", "/styles.css"
//   - ElegantOTA handler for OTA firmware updates
// ---------------------------------------------------------------------------
void connnectWSV()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(LittleFS, "/index.html", "text/html"); });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(LittleFS, "/script.js", "application/javascript"); });
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(LittleFS, "/styles.css", "text/css"); });

    // ----- BỔ SUNG ĐOẠN NÀY ĐỂ HỨNG DỮ LIỆU QUA HTTP THAY VÌ WEBSOCKET -----
    server.on("/save_wifi", HTTP_GET, [](AsyncWebServerRequest *request){
        String ssid = request->hasParam("ssid") ? request->getParam("ssid")->value() : "";
        String pass = request->hasParam("pass") ? request->getParam("pass")->value() : "";
        String server_mqtt = request->hasParam("server") ? request->getParam("server")->value() : "";
        String token = request->hasParam("token") ? request->getParam("token")->value() : "";
        String port = request->hasParam("port") ? request->getParam("port")->value() : "1883";
        
        request->send(200, "text/plain", "OK");
        
        Serial.println("\n📥 Nhận cấu hình từ HTTP Web:");
        // Hàm này sẽ tự động lưu thông tin vào LittleFS và khởi động lại ESP32
        Save_info_File(ssid, pass, token, server_mqtt, port); 
    });
    // -----------------------------------------------------------------------

    server.begin();
    ElegantOTA.begin(&server);  
    webserver_isrunning = true;
}

// ---------------------------------------------------------------------------
// Webserver_stop — Gracefully shut down the WebSocket and HTTP server.
// Called when Wi-Fi connection is lost and needs to be re-established.
// ---------------------------------------------------------------------------
void Webserver_stop()
{
    ws.closeAll();   // Disconnect all WebSocket clients cleanly
    server.end();
    webserver_isrunning = false;
}

// Timestamp of the last sensor broadcast (milliseconds since boot)
unsigned long last_broadcast = 0;

// ---------------------------------------------------------------------------
// Webserver_reconnect — Called from loop() on every iteration.
//
// Responsibilities:
//   1. Starts the server if it is not yet running (lazy init / auto-restart)
//   2. Keeps ElegantOTA processing loop alive (required for OTA to work)
//   3. Broadcasts latest sensor data (temp + humidity) to all WebSocket clients
//      every 3 seconds, reading from the mutex-protected shared struct defined
//      in global.cpp (Task 3). Skips broadcast if sensorData_read() fails
//      (mutex timeout) to avoid sending stale or zeroed data to clients.
// ---------------------------------------------------------------------------
void Webserver_reconnect()
{
    if (!webserver_isrunning)
    {
        connnectWSV();
    }
    ElegantOTA.loop();  // Must be called regularly for OTA to function

    // Broadcast sensor data to ALL connected clients every 3 seconds
    if (millis() - last_broadcast > 3000) {
        last_broadcast = millis();
        if (webserver_isrunning && ws.count() > 0) {
            // Read sensor data via mutex-protected accessor (Task 3 integration).
            // sensorData_read() acquires xMutexSensorData with a 100ms timeout,
            // copies the shared SensorData_t struct, then releases the mutex.
            SensorData_t data;
            bool hasSensor = sensorData_read(&data);

            bool led1On = false;
            bool led2On = false;
            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0;
            getLightStates(&led1On, &led2On, &r, &g, &b);

            StaticJsonDocument<384> doc;
            if (hasSensor) {
                doc["temp"] = data.temperature;
                doc["hum"] = data.humidity;
            }

            JsonObject control = doc.createNestedObject("control");
            control["mode"] = (getControlMode() == CONTROL_MODE_MANUAL) ? "MANUAL" : "AUTO";

            JsonObject led1 = control.createNestedObject("led1");
            led1["gpio"] = LED_GPIO;
            led1["status"] = led1On ? "ON" : "OFF";

            JsonObject led2 = control.createNestedObject("led2");
            led2["gpio"] = NEO_PIN;
            led2["status"] = led2On ? "ON" : "OFF";

            JsonObject color = led2.createNestedObject("color");
            color["r"] = r;
            color["g"] = g;
            color["b"] = b;

            // --- Real-time system info (purely dynamic, no hardcoded labels) ---
            JsonObject sys = doc.createNestedObject("system");

            // Chip info
            String chipModel = ESP.getChipModel();
            if (chipModel.length() > 0) {
                sys["Chip"] = chipModel;
                sys["Revision"] = String(ESP.getChipRevision());
                sys["CPU"] = String(ESP.getCpuFreqMHz()) + " MHz";
            }

            // Memory info
            sys["FreeHeap"] = String(ESP.getFreeHeap() / 1024.0, 1) + " KB";
            sys["HeapSize"] = String(ESP.getHeapSize() / 1024.0, 1) + " KB";

            // Network info
            sys["IP"] = WiFi.localIP().toString();
            sys["RSSI"] = String(WiFi.RSSI()) + " dBm";

            // Uptime (millis -> human readable)
            unsigned long uptimeSec = millis() / 1000;
            unsigned long uptimeMin = uptimeSec / 60;
            unsigned long uptimeHr = uptimeMin / 60;
            String uptimeStr = String(uptimeHr) + "h " + String(uptimeMin % 60) + "m " + String(uptimeSec % 60) + "s";
            sys["Uptime"] = uptimeStr;

            String payload;
            serializeJson(doc, payload);
            Webserver_sendata(payload);

            if (!hasSensor) {
                Serial.println("[Webserver] WARNING: Could not read sensor data for broadcast.");
            }
        }
    }
}

