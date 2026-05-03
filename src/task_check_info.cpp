#include "task_check_info.h"

void Load_info_File(AppContext_t *act) {
  File file = LittleFS.open("/info.dat", "r");
  if (!file) return;

  DynamicJsonDocument doc(4096);
  if (!deserializeJson(doc, file)) {
    act->wifi_ssid       = doc["WIFI_SSID"].as<String>();
    act->wifi_pass       = doc["WIFI_PASS"].as<String>();
    act->core_iot_token  = doc["CORE_IOT_TOKEN"].as<String>();
    act->core_iot_server = doc["CORE_IOT_SERVER"].as<String>();
    act->core_iot_port   = doc["CORE_IOT_PORT"].as<String>();
  }
  file.close();
}

void Delete_info_File() {
  Serial.println("🔄 Đang tiến hành tẩy xóa toàn bộ cấu hình...");
  
  LittleFS.remove("/info.dat");
  
  WiFi.disconnect(true, true); 
  
  Serial.println("✅ Đã xóa sạch! Mạch sẽ khởi động lại sau 1 giây...");
  Serial.flush(); 
  
  delay(1000);
  ESP.restart();
}

void Save_info_File(String ssid, String pass, String token, String server, String port, AppContext_t *act) {
  DynamicJsonDocument doc(4096);
  doc["WIFI_SSID"]       = ssid;
  doc["WIFI_PASS"]       = pass;
  doc["CORE_IOT_TOKEN"]  = token;
  doc["CORE_IOT_SERVER"] = server;
  doc["CORE_IOT_PORT"]   = port;

  // Cập nhật RAM ngay lập tức
  act->wifi_ssid       = ssid;
  act->wifi_pass       = pass;
  act->core_iot_token  = token;
  act->core_iot_server = server;
  act->core_iot_port   = port;

  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile) {
    serializeJson(doc, configFile);
    configFile.close();
  }
  ESP.restart();
}