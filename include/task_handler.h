#ifndef __TASK_HANDLER_H__
#define __TASK_HANDLER_H__

#include <Arduino.h>
#include <ArduinoJson.h>
#include "task_check_info.h"

void handleWebSocketMessage(String message);
void broadcastControlState(const char *note = nullptr);

#endif