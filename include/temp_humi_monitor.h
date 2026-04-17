#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__

#include <Arduino.h>
#include <Wire.h>
// #include "LiquidCrystal_I2C.h"
#include "DHT20.h"
#include "global.h"

// ---------------------------------------------------------------------------
// Display state thresholds — calibrated to lab environment (~27°C / ~55%)
// Trigger WARNING by breathing on sensor; CRITICAL by covering it tightly.
// ---------------------------------------------------------------------------
#define TEMP_WARNING_THRESHOLD   29.0f   // °C
#define TEMP_CRITICAL_THRESHOLD  35.0f   // °C
#define HUMI_WARNING_THRESHOLD   65.0f   // %
#define HUMI_CRITICAL_THRESHOLD  80.0f   // %

// LCD I2C address (decimal 33 = 0x21), 16 columns x 2 rows
#define LCD_I2C_ADDR  33
#define LCD_COLS      16
#define LCD_ROWS       2

void temp_humi_monitor(void *pvParameters);

#endif // __TEMP_HUMI_MONITOR__
