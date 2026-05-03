#ifndef __TASK_WEBSERVER_H__
#define __TASK_WEBSERVER_H__

#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
#include <task_handler.h>
#include "global.h"  

extern AsyncWebServer server;
extern AsyncWebSocket ws;

void Webserver_stop();
void Webserver_reconnect(AppContext_t *act);
void Webserver_sendata(String data);

#endif