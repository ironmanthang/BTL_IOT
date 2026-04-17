#include "tinyml.h"

// Globals, for the convenience of one-shot setup.
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; // Adjust size based on your model
    uint8_t tensor_arena[kTensorArenaSize];
} // namespace

void setupTinyML()
{
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); // g_model_data is from model_data.h
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void tiny_ml_task(void *pvParameters)
{

    setupTinyML();

    while (1)
    {

        // Read sensor data via mutex-protected accessor (Task 3 integration)
        SensorData_t sensorData;
        if (!sensorData_read(&sensorData)) {
            Serial.println("[TinyML] WARNING: Could not read sensor data, skipping inference.");
            vTaskDelay(1000);
            continue;
        }
        // Prepare input data from shared sensor struct
        input->data.f[0] = sensorData.temperature;
        input->data.f[1] = sensorData.humidity;

        // Run inference
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk)
        {
            error_reporter->Report("Invoke failed");
            return;
        }

        // Get and process output
        float result = output->data.f[0];
        // Serial.print("Inference result: ");
        // Serial.println(result);

        vTaskDelay(5000);
    }
}



// #include "tinyml.h"

// namespace {
//     tflite::ErrorReporter* error_reporter = nullptr;
//     const tflite::Model* model = nullptr;
//     tflite::MicroInterpreter* interpreter = nullptr;
//     TfLiteTensor* input = nullptr;
//     TfLiteTensor* output = nullptr;
//     constexpr int kTensorArenaSize = 10 * 1024;
//     uint8_t tensor_arena[kTensorArenaSize];
// }

// void setupTinyML() {
//     static tflite::MicroErrorReporter micro_error_reporter;
//     error_reporter = &micro_error_reporter;

//     model = tflite::GetModel(dht_anomaly_model_tflite);
//     if (model->version() != TFLITE_SCHEMA_VERSION) return;

//     static tflite::AllOpsResolver resolver;
//     static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
//     interpreter = &static_interpreter;

//     interpreter->AllocateTensors();
//     input = interpreter->input(0);
//     output = interpreter->output(0);
// }

// void tiny_ml_task(void *pvParameters) {
//     setupTinyML();
//     SensorData_t sensorData;

//     while (1) {
//         if (sensorData_read(&sensorData)) {
//             input->data.f[0] = sensorData.temperature;
//             input->data.f[1] = sensorData.humidity;

//             if (interpreter->Invoke() == kTfLiteOk) {
//                 float result = output->data.f[0];
//                 Serial.printf("Inference: %.2f\n", result);
//             }
//         }
//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }
// }