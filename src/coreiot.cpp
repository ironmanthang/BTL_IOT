#include "coreiot.h"
#include "task_handler.h"

void coreiot_task(void *pvParameters) {
    AppContext_t * act = (AppContext_t *)pvParameters;

    if (act->xBinarySemaphoreInternet != NULL) {
        xSemaphoreTake(act->xBinarySemaphoreInternet, portMAX_DELAY);
    }

    WiFiClient espClient;
    PubSubClient client(espClient);
    
    // Lấy thông tin Server và Port từ act thay vì gọi biến toàn cục
    client.setServer(act->core_iot_server.c_str(), act->core_iot_port.toInt());

    // Capture Lambda: [act] để hàm vô danh nhận biết được con trỏ
    client.setCallback([act](char* topic, byte* payload, unsigned int length) {
        char message[length + 1];
        memcpy(message, payload, length);
        message[length] = '\0';

        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, message)) return;

        const char* method = doc["method"];
        bool state = doc["params"].as<bool>(); 

        if (method != nullptr) {
            setControlMode(act, CONTROL_MODE_MANUAL);

            if (strcmp(method, "setLedSwitchValue") == 0) {
                setLed1State(act, state); 
                pinMode(48, OUTPUT); 
                digitalWrite(48, state ? HIGH : LOW); 
            } 
            else if (strcmp(method, "setNeoSwitchValue") == 0) {
                if (state) setLed2Color(act, 0, 0, 255); 
                else       setLed2Color(act, 0, 0, 0);   

                if (act->xSemaphoreNeoChange != NULL) {
                    xSemaphoreGive(act->xSemaphoreNeoChange);
                }
            }
            broadcastControlState(act, "CLOUD_RPC_UPDATE");
        }
    });

    TickType_t last_publish_time = 0;

    while(1) {
        if (!client.connected()) {
            String clientId = "ESP32Client-" + String(random(0xffff), HEX);
            if (client.connect(clientId.c_str(), act->core_iot_token.c_str(), NULL)) {
                client.subscribe("v1/devices/me/rpc/request/+"); 
            } else {
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue; 
            }
        }
        
        client.loop(); 

        if ((xTaskGetTickCount() - last_publish_time) > pdMS_TO_TICKS(10000)) {
            float temp = 0.0, humi = 0.0;
            bool has_data = false;

            if (act->xMutexSensorData != NULL) {
                if (xSemaphoreTake(act->xMutexSensorData, pdMS_TO_TICKS(100)) == pdTRUE) {
                    temp = act->sensorData.temperature;
                    humi = act->sensorData.humidity;
                    has_data = true;
                    xSemaphoreGive(act->xMutexSensorData);
                }
            }

            if (has_data) {
                String payload = "{\"temperature\":" + String(temp) + ",\"humidity\":" + String(humi) + "}";
                client.publish("v1/devices/me/telemetry", payload.c_str());
            }
            last_publish_time = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}