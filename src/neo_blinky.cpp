#include "neo_blinky.h"
#include "global.h"

extern Adafruit_NeoPixel pixels;

void neo_blinky(void *pvParameters) {
    pixels.begin();
    pixels.clear();
    pixels.show();
    SensorData_t data;

    while(1) {
        if (xSemaphoreTake(xSemaphoreNeoChange, portMAX_DELAY) == pdTRUE) {
            
            if (sensorData_read(&data)) {
                if (data.state == STATE_NORMAL) {
                    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
                } else if (data.state == STATE_WARNING) {
                    pixels.setPixelColor(0, pixels.Color(255, 255, 0));
                } else if (data.state == STATE_CRITICAL) {
                    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
                }
                pixels.show();
            }
        }
    }
}