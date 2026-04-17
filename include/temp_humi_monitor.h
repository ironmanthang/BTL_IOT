#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__

#include <Arduino.h>
#include <Wire.h>
#include "DHT20.h"
#include "global.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------------------------------------------------------------------
// Display state thresholds — calibrated to lab environment (~27°C / ~55%)
// Trigger WARNING by breathing on sensor; CRITICAL by covering it tightly.
// ---------------------------------------------------------------------------
#define TEMP_WARNING_THRESHOLD   29.0f   
#define TEMP_CRITICAL_THRESHOLD  35.0f   
#define HUMI_WARNING_THRESHOLD   65.0f   
#define HUMI_CRITICAL_THRESHOLD  80.0f   

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 

void temp_humi_monitor(void *pvParameters);

#endif // __TEMP_HUMI_MONITOR__
