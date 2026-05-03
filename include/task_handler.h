#ifndef __TASK_HANDLER_H__
#define __TASK_HANDLER_H__

#include <Arduino.h>
#include <ArduinoJson.h>
#include "task_check_info.h"
#include "global.h"

void handleWebSocketMessage(String message, AppContext_t *act);

// Cập nhật tham số act vào đây để coreiot.cpp gọi không bị lỗi
void broadcastControlState(AppContext_t *act, const char *note = nullptr);

#endif