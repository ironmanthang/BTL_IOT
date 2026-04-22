#include "temp_humi_monitor.h"

void temp_humi_monitor(void *pvParameters) {
    AppContext_t * act = (AppContext_t*)pvParameters;
    DisplayState_t last_state = STATE_NORMAL;

    while(1) {
        act->dht->read();
        float temp = act->dht->getTemperature();
        float humi = act->dht->getHumidity();

        sensorData_write(temp, humi, STATE_NORMAL);

        DisplayState_t current_state = STATE_NORMAL;
        if (temp >= TEMP_CRITICAL_THRESHOLD || humi >= HUMI_CRITICAL_THRESHOLD) {
            current_state = STATE_CRITICAL;
        } else if (temp >= TEMP_WARNING_THRESHOLD || humi >= HUMI_WARNING_THRESHOLD) {
            current_state = STATE_WARNING;
        }


        if (act->xMutexSensorData != NULL) {
            if (xSemaphoreTake(act->xMutexSensorData, portMAX_DELAY) == pdTRUE) {
                act->sensorData.temperature = temp;
                act->sensorData.humidity = humi;
                act->sensorData.state = current_state;
                xSemaphoreGive(act->xMutexSensorData); // Ghi xong trả chìa khóa
            }
        }

        if (current_state != last_state) {
            if(act->xSemaphoreStateChange != NULL) xSemaphoreGive(act->xSemaphoreStateChange);
            if(act->xSemaphoreNeoChange != NULL)   xSemaphoreGive(act->xSemaphoreNeoChange);
            last_state = current_state;
        }

        act->lcd->clearDisplay();

        act->lcd->setCursor(0, 0);
        act->lcd->printf("T:%.1fC %s ", temp, current_state == STATE_CRITICAL ? "CRIT" : (current_state == STATE_WARNING ? "WARN" : "NORM"));
        
        act->lcd->setCursor(0, 16); 
        act->lcd->printf("H:%.1f%%   ", humi);

        act->lcd->display(); 

        // Serial.print("nhiet do :");
        // Serial.print(temp);
        // Serial.println(" C");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}