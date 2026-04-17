#include "coreiot.h"
#include "global.h"
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

void coreiot_task(void *pvParameters) {
    // 1. Mở hộp lấy tài nguyên
    AppContext_t * act = (AppContext_t *)pvParameters;

    // 2. Chờ kết nối WiFi (Lấy Semaphore 1 lần duy nhất lúc khởi động)
    if (act->xBinarySemaphoreInternet != NULL) {
        Serial.println("[CoreIoT] Đang chờ WiFi...");
        xSemaphoreTake(act->xBinarySemaphoreInternet, portMAX_DELAY);
        Serial.println("[CoreIoT] Đã có WiFi! Bắt đầu chạy MQTT.");
    }

    // 3. Khởi tạo Client mạng NGAY BÊN TRONG TASK (Không dùng Global)
    WiFiClient espClient;
    PubSubClient client(espClient);

    // THÔNG SỐ SERVER (Bạn có thể đưa vào global.h nếu muốn, ở đây để nội bộ cho gọn)
    const char* mqtt_server = "125.212.240.243"; 
    const char* mqtt_token  = "7113embizkufhmz4qcff"; 
    const int   mqtt_port   = 1883;

    client.setServer(mqtt_server, mqtt_port);

    // =========================================================================
    // 4. HÀM CALLBACK NHẬN LỆNH TỪ CLOUD (Dùng Lambda Function để bắt biến act)
    // =========================================================================
    client.setCallback([act](char* topic, byte* payload, unsigned int length) {
        Serial.printf("Message arrived [%s]\n", topic);
        
        char message[length + 1];
        memcpy(message, payload, length);
        message[length] = '\0';
        Serial.printf("Payload: %s\n", message);

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, message);

        if (error) {
            Serial.println("deserializeJson() failed");
            return;
        }

        const char* method = doc["method"];
        if (method != nullptr && strcmp(method, "setStateLED") == 0) {
            const char* params = doc["params"];
            if (params != nullptr && strcmp(params, "ON") == 0) {
                Serial.println("Cloud ra lệnh: BẬT THIẾT BỊ (ON)");
                
                // TODO: BẠN CÓ THỂ ĐIỀU KHIỂN LED QUA act Ở ĐÂY
                // Ví dụ: act->pixels->fill(act->pixels->Color(255, 255, 255));
                //        act->pixels->show();
                
            } else {   
                Serial.println("Cloud ra lệnh: TẮT THIẾT BỊ (OFF)");
                
                // TODO: TẮT LED QUA act
                // Ví dụ: act->pixels->clear();
                //        act->pixels->show();
            }
        }
    });

    // Biến lưu thời gian để gửi dữ liệu mỗi 10 giây
    TickType_t last_publish_time = 0;

    // =========================================================================
    // 5. VÒNG LẶP CHÍNH CỦA TASK IOT
    // =========================================================================
    while(1) {
        
        // KIỂM TRA VÀ KẾT NỐI LẠI MQTT
        if (!client.connected()) {
            Serial.print("[CoreIoT] Đang kết nối MQTT...");
            String clientId = "ESP32Client-" + String(random(0xffff), HEX);

            // Chú ý: Server thường yêu cầu Token làm Username
            if (client.connect(clientId.c_str(), mqtt_token, NULL)) {
                Serial.println("Thành công!");
                client.subscribe("v1/devices/me/rpc/request/+"); // Đăng ký nhận lệnh
            } else {
                Serial.print("Thất bại, rc=");
                Serial.print(client.state());
                Serial.println(" - Thử lại sau 5s");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue; // Lỗi thì quay lại đầu vòng lặp
            }
        }
        
        // Lệnh sống còn: Giúp client duy trì kết nối và đọc tin nhắn Callback
        client.loop(); 

        // GỬI TELEMETRY MỖI 10 GIÂY (Không dùng Delay để tránh kẹt client.loop)
        if ((xTaskGetTickCount() - last_publish_time) > pdMS_TO_TICKS(10000)) {
            
            float temp = 0.0;
            float humi = 0.0;
            bool has_data = false;

            // Mở hộp lấy dữ liệu an toàn
            if (act->xMutexSensorData != NULL) {
                if (xSemaphoreTake(act->xMutexSensorData, portMAX_DELAY) == pdTRUE) {
                    temp = act->sensorData.temperature;
                    humi = act->sensorData.humidity;
                    has_data = true;
                    xSemaphoreGive(act->xMutexSensorData);
                }
            }

            if (has_data) {
                // Đóng gói JSON thủ công giống code của bạn
                String payload = "{\"temperature\":" + String(temp) + ",\"humidity\":" + String(humi) + "}";
                client.publish("v1/devices/me/telemetry", payload.c_str());
                Serial.println("Đã gửi Cloud: " + payload);
            } else {
                Serial.println("[CoreIOT] WARNING: Không lấy được dữ liệu cảm biến.");
            }

            // Cập nhật lại thời gian vừa gửi
            last_publish_time = xTaskGetTickCount();
        }

        // Chạy vòng lặp rất nhanh để client.loop() luôn mượt mà (chỉ nghỉ 50ms)
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}