#include "tinyml.h"
#include "global.h" 

namespace {
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; // Cẩn thận với bộ nhớ RAM trên ESP32
    uint8_t tensor_arena[kTensorArenaSize];
}

void tiny_ml_task(void *pvParameters) {
    AppContext_t * act = (AppContext_t *)pvParameters;

    Serial.println("TensorFlow Lite Init....");
    
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); 
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        error_reporter->Report("Model schema mismatch!");
        vTaskDelete(NULL); // Dừng task nếu file model bị lỗi
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        error_reporter->Report("AllocateTensors() failed");
        vTaskDelete(NULL);
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
    // ========================================================


    while (1) {
        float temp = 0.0;
        float humi = 0.0;
        bool data_ready = false;

        if (act->xMutexSensorData != NULL) {
            if (xSemaphoreTake(act->xMutexSensorData, portMAX_DELAY) == pdTRUE) {
                
                temp = act->sensorData.temperature;
                humi = act->sensorData.humidity;
                
                // Đảm bảo cảm biến đã đo được số liệu thực tế chứ không phải số 0 mặc định
                if (temp > 0.0 && humi > 0.0) {
                    data_ready = true;
                }
                
                xSemaphoreGive(act->xMutexSensorData); // Nhớ trả chìa khóa
            }
        }

        if (!data_ready) {
            Serial.println("[TinyML] WARNING: Could not read sensor data, skipping inference.");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue; 
        }

        input->data.f[0] = temp;
        input->data.f[1] = humi;

        if (interpreter->Invoke() != kTfLiteOk) {
            error_reporter->Report("Invoke failed");
            continue; // Lỗi thì thử lại ở vòng lặp sau thay vì văng luôn
        }

        float result = output->data.f[0];
        Serial.print("Inference result: ");
        Serial.println(result);

        sensorData_update_ai(result);

        if (act->xMutexSensorData != NULL) {
            if (xSemaphoreTake(act->xMutexSensorData, portMAX_DELAY) == pdTRUE) {
                act->sensorData.ai_score = result;
                xSemaphoreGive(act->xMutexSensorData); // Ghi xong trả chìa khóa
            }
        }

        vTaskDelay(5000);
    }
}