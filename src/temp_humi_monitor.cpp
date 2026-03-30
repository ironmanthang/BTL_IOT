#include "temp_humi_monitor.h"

// ---------------------------------------------------------------------------
// Module-local objects — not global, owned exclusively by this task
// ---------------------------------------------------------------------------
static DHT20             dht20;
static LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

// ---------------------------------------------------------------------------
// evaluateDisplayState — Pure function, O(1).
// Evaluates thresholds in CRITICAL-first order so the worst state always wins.
// ---------------------------------------------------------------------------
static DisplayState_t evaluateDisplayState(float temp, float humidity) {
    // Sensor error: treat as CRITICAL so it is never silently ignored
    if (isnan(temp) || isnan(humidity) || temp < 0 || humidity < 0) {
        return STATE_CRITICAL;
    }
    if (temp > TEMP_CRITICAL_THRESHOLD || humidity > HUMI_CRITICAL_THRESHOLD) {
        return STATE_CRITICAL;
    }
    if (temp >= TEMP_WARNING_THRESHOLD || humidity >= HUMI_WARNING_THRESHOLD) {
        return STATE_WARNING;
    }
    return STATE_NORMAL;
}

// ---------------------------------------------------------------------------
// updateLCD — Renders current readings and state onto the 16x2 display.
//
// Line 1 format (16 chars max):
//   STATE_NORMAL   → "S: NORMAL       "
//   STATE_WARNING  → "S: WARNING!     "
//   STATE_CRITICAL → "S: CRITICAL!!   "
//
// Line 2 format (16 chars max):
//   "T:26.9C  H:55.5%"   (fits exactly in 16 chars with typical readings)
// ---------------------------------------------------------------------------
static void updateLCD(float temp, float humidity, DisplayState_t state) {
    // --- Line 1: State label ---
    lcd.setCursor(0, 0);
    lcd.print("S: ");
    switch (state) {
        case STATE_NORMAL:
            lcd.print("NORMAL     ");  // trailing spaces clear stale chars
            break;
        case STATE_WARNING:
            lcd.print("WARNING!   ");
            break;
        case STATE_CRITICAL:
            lcd.print("CRITICAL!! ");
            break;
    }

    // --- Line 2: Sensor values ---
    // Format: "T:XX.XC  H:XX.X%"
    // Use dtostrf for fixed-width float formatting (avoids String heap fragmentation)
    char line2[17];  // 16 chars + null terminator
    char tBuf[6];    // "-XX.X" worst case
    char hBuf[6];

    dtostrf(temp,    4, 1, tBuf);  // width=4, 1 decimal
    dtostrf(humidity, 4, 1, hBuf);

    snprintf(line2, sizeof(line2), "T:%sC H:%s%%", tBuf, hBuf);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

// ---------------------------------------------------------------------------
// temp_humi_monitor — RTOS Task Entry Point
// Reads DHT20 every 5 seconds, evaluates display state, updates LCD and
// shared sensor data via mutex-protected sensorData_write().
// ---------------------------------------------------------------------------
void temp_humi_monitor(void *pvParameters) {

    // ---- Hardware init ----
    Wire.begin(11, 12);     // SDA = GPIO 11, SCL = GPIO 12 (shared I2C bus)
    Serial.begin(115200);
    dht20.begin();

    // ---- LCD init ----
    // This library uses begin() — NOT init() (see lib/LCD/LiquidCrystal_I2C.h)
    lcd.begin();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");
    Serial.println("[TempHumi] LCD initialized.");

    // Track previous values to avoid unnecessary LCD redraws
    float prevTemp  = -999.0f;
    float prevHumi  = -999.0f;
    DisplayState_t prevState = STATE_NORMAL;
    bool firstRead  = true;

    while (1) {
        // ---- Read sensor ----
        dht20.read();
        float temperature = dht20.getTemperature();
        float humidity    = dht20.getHumidity();

        // ---- Handle read failure ----
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("[TempHumi] ERROR: Failed to read DHT20 sensor!");
            temperature = -1.0f;
            humidity    = -1.0f;
        }

        // ---- Evaluate display state ----
        DisplayState_t currentState = evaluateDisplayState(temperature, humidity);

        // ---- Write to shared data (replaces glob_temperature / glob_humidity) ----
        if (!sensorData_write(temperature, humidity, currentState)) {
            Serial.println("[TempHumi] WARNING: Could not write sensor data (mutex timeout).");
        }

        // ---- Update LCD only when values change meaningfully (>0.1 delta) ----
        // This reduces unnecessary I2C traffic on the shared bus.
        bool valuesChanged = firstRead
            || (fabsf(temperature - prevTemp) > 0.1f)
            || (fabsf(humidity    - prevHumi) > 0.1f)
            || (currentState != prevState);

        if (valuesChanged) {
            if (temperature < 0 || humidity < 0) {
                // Sensor error — show dedicated error message on LCD
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("SENSOR ERROR!   ");
                lcd.setCursor(0, 1);
                lcd.print("Check DHT20 wir.");
            } else {
                updateLCD(temperature, humidity, currentState);
            }
            prevTemp  = temperature;
            prevHumi  = humidity;
            prevState = currentState;
            firstRead = false;
        }

        // ---- Serial debug output ----
        Serial.printf("[TempHumi] T:%.2f°C  H:%.2f%%  State:%s\n",
            temperature, humidity,
            (currentState == STATE_NORMAL)   ? "NORMAL" :
            (currentState == STATE_WARNING)  ? "WARNING" : "CRITICAL");

        vTaskDelay(pdMS_TO_TICKS(5000));  // Read every 5 seconds
    }
}