#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// WiFi настройки
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Серверы
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Структура зарядной станции (полная версия)
struct ChargingStation {
  int id;
  String displayName;
  String technicalName;
  String type;
  String status;
  String description;
  String ipAddress;
  float maxPower;
  float currentPower;
  float chargerPower;
  float masterAvailablePower;
  
  // Электрические параметры
  float voltagePhase1;
  float voltagePhase2;
  float voltagePhase3;
  float currentPhase1;
  float currentPhase2;
  float currentPhase3;
  
  // Состояния подключений
  bool carConnection;
  bool carChargingPermission;
  bool carError;
  bool masterOnline;
  bool masterChargingPermission;
  
  // Дополнительные параметры
  bool singlePhaseConnection;
  bool powerOverconsumption;
  bool fixedPower;
  
  // Метки времени
  unsigned long lastUpdate;
};

// Расширенная конфигурация для 16MB
#define MAX_STATIONS 50
#define MAX_CONCURRENT_CLIENTS 20
#define WEBSOCKET_UPDATE_INTERVAL 5000
#define JSON_BUFFER_SIZE 8192

// Глобальные переменные
ChargingStation stations[MAX_STATIONS];
int stationCount = 0;
unsigned long lastWebSocketUpdate = 0;
bool systemInitialized = false;

// Логирование
struct LogEntry {
  unsigned long timestamp;
  String level;
  String message;
};

#define MAX_LOG_ENTRIES 100
LogEntry systemLogs[MAX_LOG_ENTRIES];
int logIndex = 0;

void setup() {
  Serial.begin(115200);
  
  // Инициализация LittleFS
  if (!LittleFS.begin(true)) {
    logMessage("ERROR", "Ошибка монтирования LittleFS");
    return;
  }
  
  logMessage("INFO", "LittleFS инициализирована успешно");
  
  // Проверка доступной памяти
  logMessage("INFO", "Общая память Flash: " + String(ESP.getFlashChipSize() / (1024 * 1024)) + "MB");
  logMessage("INFO", "Доступная память PSRAM: " + String(ESP.getPsramSize() / (1024 * 1024)) + "MB");
  
  // Подключение к WiFi
  connectToWiFi();
  
  // Загрузка конфигурации и данных
  loadSystemConfiguration();
  loadStations();
  
  // Настройка веб-сервера
  setupWebServer();
  
  // Настройка WebSocket
  setupWebSocket();
  
  // Запуск серверов
  server.begin();
  webSocket.begin();
  
  systemInitialized = true;
  logMessage("INFO", "Система полностью инициализирована");
  logMessage("INFO", "Веб-интерфейс доступен по адресу: http://" + WiFi.localIP().toString());
}

void loop() {
  webSocket.loop();
  
  // Периодическое обновление через WebSocket
  if (millis() - lastWebSocketUpdate > WEBSOCKET_UPDATE_INTERVAL) {
    broadcastStationUpdates();
    lastWebSocketUpdate = millis();
  }
  
  // Мониторинг памяти
  static unsigned long lastMemCheck = 0;
  if (millis() - lastMemCheck > 60000) { // Каждую минуту
    checkMemoryUsage();
    lastMemCheck = millis();
  }
  
  delay(10);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  logMessage("INFO", "Подключение к WiFi: " + String(ssid));
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    logMessage("INFO", "WiFi подключен! IP: " + WiFi.localIP().toString());
    logMessage("INFO", "Сила сигнала: " + String(WiFi.RSSI()) + " dBm");
  } else {
    logMessage("ERROR", "Не удалось подключиться к WiFi");
    // Создаем точку доступа
    createAccessPoint();
  }
}

void createAccessPoint() {
  WiFi.softAP("ESP32-ChargingStation-16MB", "charging2025");
  logMessage("INFO", "Создана точка доступа: ESP32-ChargingStation-16MB");
  logMessage("INFO", "IP точки доступа: " + WiFi.softAPIP().toString());
}

void setupWebServer() {
  // Статические файлы (React build)
  server.serveStatic("/", LittleFS, "/dist/").setDefaultFile("index.html");
  
  // API маршруты
  setupAPIRoutes();
  
  // WebSocket для real-time обновлений
  server.on("/ws", HTTP_GET, [](AsyncWebServerRequest *request){
    // WebSocket обработка через отдельный сервер
  });
  
  // Обработчик 404
  server.onNotFound([](AsyncWebServerRequest *request){
    if (request->url().startsWith("/api/")) {
      request->send(404, "application/json", "{\"error\":\"API endpoint not found\"}");
    } else {
      // Для SPA - отдаем index.html
      request->send(LittleFS, "/dist/index.html", "text/html");
    }
  });
  
  logMessage("INFO", "Веб-сервер настроен");
}

void setupAPIRoutes() {
  // Получение всех станций
  server.on("/api/stations", HTTP_GET, [](AsyncWebServerRequest *request){
    handleGetStations(request);
  });
  
  // Получение конкретной станции
  server.on("/api/stations", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("id")) {
      int id = request->getParam("id")->value().toInt();
      handleGetStation(request, id);
    } else {
      handleGetStations(request);
    }
  });
  
  // Создание станции
  server.on("/api/stations", HTTP_POST, [](AsyncWebServerRequest *request){}, 
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    handleCreateStation(request, data, len);
  });
  
  // Обновление станции
  server.on("/api/stations/update", HTTP_POST, [](AsyncWebServerRequest *request){}, 
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    handleUpdateStation(request, data, len);
  });
  
  // PATCH для обновления отдельных полей
  server.on("/api/stations/([0-9]+)", HTTP_PATCH, [](AsyncWebServerRequest *request){}, 
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    String idStr = request->pathArg(0);
    int id = idStr.toInt();
    handlePatchStation(request, id, data, len);
  });
  
  // Удаление станции
  server.on("/api/stations/([0-9]+)", HTTP_DELETE, [](AsyncWebServerRequest *request){
    String idStr = request->pathArg(0);
    int id = idStr.toInt();
    handleDeleteStation(request, id);
  });
  
  // ESP32 сканирование
  server.on("/api/esp32/scan", HTTP_POST, [](AsyncWebServerRequest *request){
    handleESP32Scan(request);
  });
  
  // Системная информация
  server.on("/api/system/info", HTTP_GET, [](AsyncWebServerRequest *request){
    handleSystemInfo(request);
  });
  
  // Логи системы
  server.on("/api/system/logs", HTTP_GET, [](AsyncWebServerRequest *request){
    handleSystemLogs(request);
  });
  
  // OTA обновления
  server.on("/api/system/update", HTTP_POST, [](AsyncWebServerRequest *request){}, 
    [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    handleOTAUpdate(request, filename, index, data, len, final);
  });
}

void setupWebSocket() {
  webSocket.onEvent(webSocketEvent);
  logMessage("INFO", "WebSocket сервер настроен на порту 81");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      logMessage("INFO", "WebSocket клиент " + String(num) + " отключился");
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      logMessage("INFO", "WebSocket клиент " + String(num) + " подключился с " + ip.toString());
      
      // Отправляем текущее состояние новому клиенту
      sendStationsToClient(num);
      break;
    }
    
    case WStype_TEXT:
      logMessage("INFO", "WebSocket сообщение от клиента " + String(num) + ": " + String((char*)payload));
      handleWebSocketMessage(num, (char*)payload);
      break;
      
    default:
      break;
  }
}

void handleGetStations(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  JsonArray array = doc.to<JsonArray>();
  
  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.createNestedObject();
    populateStationJson(station, stations[i]);
  }
  
  String response;
  serializeJson(doc, response);
  
  request->send(200, "application/json", response);
  logMessage("DEBUG", "Отправлены данные " + String(stationCount) + " станций");
}

void handleGetStation(AsyncWebServerRequest *request, int id) {
  for (int i = 0; i < stationCount; i++) {
    if (stations[i].id == id) {
      DynamicJsonDocument doc(2048);
      JsonObject station = doc.to<JsonObject>();
      populateStationJson(station, stations[i]);
      
      String response;
      serializeJson(doc, response);
      
      request->send(200, "application/json", response);
      logMessage("DEBUG", "Отправлены данные станции ID " + String(id));
      return;
    }
  }
  
  request->send(404, "application/json", "{\"error\":\"Станция не найдена\"}");
  logMessage("WARNING", "Запрошена несуществующая станция ID " + String(id));
}

void handleCreateStation(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
  if (stationCount >= MAX_STATIONS) {
    request->send(400, "application/json", "{\"error\":\"Достигнут лимит станций\"}");
    return;
  }
  
  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, data, len) != DeserializationError::Ok) {
    request->send(400, "application/json", "{\"error\":\"Неверный JSON\"}");
    return;
  }
  
  // Создание новой станции
  ChargingStation newStation = {};
  newStation.id = getNextStationId();
  newStation.displayName = doc["displayName"].as<String>();
  newStation.technicalName = doc["technicalName"].as<String>();
  newStation.type = doc["type"].as<String>();
  newStation.status = "available";
  newStation.maxPower = doc["maxPower"];
  newStation.currentPower = 0.0;
  newStation.lastUpdate = millis();
  
  // Дополнительные поля
  if (doc.containsKey("description")) {
    newStation.description = doc["description"].as<String>();
  }
  if (doc.containsKey("ipAddress")) {
    newStation.ipAddress = doc["ipAddress"].as<String>();
  }
  
  stations[stationCount] = newStation;
  stationCount++;
  
  // Сохранение
  saveStations();
  
  // Уведомление через WebSocket
  broadcastStationUpdate(newStation);
  
  request->send(201, "application/json", "{\"message\":\"Станция создана\", \"id\":" + String(newStation.id) + "}");
  logMessage("INFO", "Создана новая станция: " + newStation.displayName);
}

void handleUpdateStation(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, data, len) != DeserializationError::Ok) {
    request->send(400, "application/json", "{\"error\":\"Неверный JSON\"}");
    return;
  }
  
  int stationId = doc["id"];
  
  for (int i = 0; i < stationCount; i++) {
    if (stations[i].id == stationId) {
      updateStationFromJson(stations[i], doc);
      stations[i].lastUpdate = millis();
      
      saveStations();
      broadcastStationUpdate(stations[i]);
      
      request->send(200, "application/json", "{\"message\":\"Станция обновлена\"}");
      logMessage("INFO", "Обновлена станция ID " + String(stationId));
      return;
    }
  }
  
  request->send(404, "application/json", "{\"error\":\"Станция не найдена\"}");
}

void handlePatchStation(AsyncWebServerRequest *request, int id, uint8_t *data, size_t len) {
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, data, len) != DeserializationError::Ok) {
    request->send(400, "application/json", "{\"error\":\"Неверный JSON\"}");
    return;
  }
  
  for (int i = 0; i < stationCount; i++) {
    if (stations[i].id == id) {
      updateStationFromJson(stations[i], doc, true); // Селективное обновление
      stations[i].lastUpdate = millis();
      
      saveStations();
      broadcastStationUpdate(stations[i]);
      
      request->send(200, "application/json", "{\"message\":\"Станция обновлена\"}");
      logMessage("INFO", "PATCH обновление станции ID " + String(id));
      return;
    }
  }
  
  request->send(404, "application/json", "{\"error\":\"Станция не найдена\"}");
}

void handleDeleteStation(AsyncWebServerRequest *request, int id) {
  for (int i = 0; i < stationCount; i++) {
    if (stations[i].id == id) {
      // Сдвигаем массив
      for (int j = i; j < stationCount - 1; j++) {
        stations[j] = stations[j + 1];
      }
      stationCount--;
      
      saveStations();
      broadcastStationDeleted(id);
      
      request->send(200, "application/json", "{\"message\":\"Станция удалена\"}");
      logMessage("INFO", "Удалена станция ID " + String(id));
      return;
    }
  }
  
  request->send(404, "application/json", "{\"error\":\"Станция не найдена\"}");
}

void handleESP32Scan(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Добавляем текущую ESP32
  JsonObject board = array.createNestedObject();
  board["id"] = "ESP32-16MB-" + WiFi.macAddress();
  board["type"] = "master";
  board["ip"] = WiFi.localIP().toString();
  board["name"] = "ESP32 16MB Charging Station";
  board["status"] = "online";
  board["lastSeen"] = "now";
  board["memory"] = "16MB";
  board["stations"] = stationCount;
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
  
  logMessage("INFO", "Выполнено сканирование ESP32");
}

void handleSystemInfo(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(1024);
  
  doc["chipModel"] = ESP.getChipModel();
  doc["chipRevision"] = ESP.getChipRevision();
  doc["cpuFreq"] = ESP.getCpuFreqMHz();
  doc["flashSize"] = ESP.getFlashChipSize();
  doc["flashSpeed"] = ESP.getFlashChipSpeed();
  doc["psramSize"] = ESP.getPsramSize();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["freePsram"] = ESP.getFreePsram();
  doc["uptime"] = millis();
  doc["wifiRSSI"] = WiFi.RSSI();
  doc["stationCount"] = stationCount;
  doc["maxStations"] = MAX_STATIONS;
  doc["version"] = "ESP32-16MB-v1.0";
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void handleSystemLogs(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(4096);
  JsonArray array = doc.to<JsonArray>();
  
  for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
    int idx = (logIndex + i) % MAX_LOG_ENTRIES;
    if (systemLogs[idx].timestamp > 0) {
      JsonObject log = array.createNestedObject();
      log["timestamp"] = systemLogs[idx].timestamp;
      log["level"] = systemLogs[idx].level;
      log["message"] = systemLogs[idx].message;
    }
  }
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void handleOTAUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    logMessage("INFO", "OTA обновление началось");
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      return;
    }
  }
  
  if (Update.write(data, len) != len) {
    Update.printError(Serial);
    return;
  }
  
  if (final) {
    if (Update.end(true)) {
      logMessage("INFO", "OTA обновление завершено успешно");
      request->send(200, "application/json", "{\"message\":\"Обновление успешно, перезагрузка...\"}");
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
      request->send(500, "application/json", "{\"error\":\"Ошибка OTA обновления\"}");
    }
  }
}

// Вспомогательные функции

void populateStationJson(JsonObject& json, const ChargingStation& station) {
  json["id"] = station.id;
  json["displayName"] = station.displayName;
  json["technicalName"] = station.technicalName;
  json["type"] = station.type;
  json["status"] = station.status;
  json["description"] = station.description;
  json["ipAddress"] = station.ipAddress;
  json["maxPower"] = station.maxPower;
  json["currentPower"] = station.currentPower;
  json["chargerPower"] = station.chargerPower;
  json["masterAvailablePower"] = station.masterAvailablePower;
  
  json["voltagePhase1"] = station.voltagePhase1;
  json["voltagePhase2"] = station.voltagePhase2;
  json["voltagePhase3"] = station.voltagePhase3;
  json["currentPhase1"] = station.currentPhase1;
  json["currentPhase2"] = station.currentPhase2;
  json["currentPhase3"] = station.currentPhase3;
  
  json["carConnection"] = station.carConnection;
  json["carChargingPermission"] = station.carChargingPermission;
  json["carError"] = station.carError;
  json["masterOnline"] = station.masterOnline;
  json["masterChargingPermission"] = station.masterChargingPermission;
  
  json["singlePhaseConnection"] = station.singlePhaseConnection;
  json["powerOverconsumption"] = station.powerOverconsumption;
  json["fixedPower"] = station.fixedPower;
  
  json["lastUpdate"] = station.lastUpdate;
}

void updateStationFromJson(ChargingStation& station, JsonObject& json, bool selective = false) {
  if (json.containsKey("displayName")) station.displayName = json["displayName"].as<String>();
  if (json.containsKey("technicalName")) station.technicalName = json["technicalName"].as<String>();
  if (json.containsKey("type")) station.type = json["type"].as<String>();
  if (json.containsKey("status")) station.status = json["status"].as<String>();
  if (json.containsKey("description")) station.description = json["description"].as<String>();
  if (json.containsKey("ipAddress")) station.ipAddress = json["ipAddress"].as<String>();
  
  if (json.containsKey("maxPower")) station.maxPower = json["maxPower"];
  if (json.containsKey("currentPower")) station.currentPower = json["currentPower"];
  if (json.containsKey("chargerPower")) station.chargerPower = json["chargerPower"];
  if (json.containsKey("masterAvailablePower")) station.masterAvailablePower = json["masterAvailablePower"];
  
  if (json.containsKey("voltagePhase1")) station.voltagePhase1 = json["voltagePhase1"];
  if (json.containsKey("voltagePhase2")) station.voltagePhase2 = json["voltagePhase2"];
  if (json.containsKey("voltagePhase3")) station.voltagePhase3 = json["voltagePhase3"];
  if (json.containsKey("currentPhase1")) station.currentPhase1 = json["currentPhase1"];
  if (json.containsKey("currentPhase2")) station.currentPhase2 = json["currentPhase2"];
  if (json.containsKey("currentPhase3")) station.currentPhase3 = json["currentPhase3"];
  
  if (json.containsKey("carConnection")) station.carConnection = json["carConnection"];
  if (json.containsKey("carChargingPermission")) station.carChargingPermission = json["carChargingPermission"];
  if (json.containsKey("carError")) station.carError = json["carError"];
  if (json.containsKey("masterOnline")) station.masterOnline = json["masterOnline"];
  if (json.containsKey("masterChargingPermission")) station.masterChargingPermission = json["masterChargingPermission"];
  
  if (json.containsKey("singlePhaseConnection")) station.singlePhaseConnection = json["singlePhaseConnection"];
  if (json.containsKey("powerOverconsumption")) station.powerOverconsumption = json["powerOverconsumption"];
  if (json.containsKey("fixedPower")) station.fixedPower = json["fixedPower"];
}

void loadSystemConfiguration() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    logMessage("WARNING", "Конфигурационный файл не найден, создаем стандартный");
    createDefaultConfiguration();
    return;
  }
  
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, configFile) == DeserializationError::Ok) {
    // Загружаем настройки системы
    logMessage("INFO", "Конфигурация загружена успешно");
  } else {
    logMessage("ERROR", "Ошибка парсинга конфигурации");
    createDefaultConfiguration();
  }
  
  configFile.close();
}

void loadStations() {
  File file = LittleFS.open("/stations.json", "r");
  if (!file) {
    logMessage("WARNING", "Файл станций не найден, создаем тестовые данные");
    createDefaultStations();
    return;
  }
  
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  if (deserializeJson(doc, file) != DeserializationError::Ok) {
    logMessage("ERROR", "Ошибка парсинга JSON станций");
    file.close();
    createDefaultStations();
    return;
  }
  
  file.close();
  
  JsonArray array = doc.as<JsonArray>();
  stationCount = 0;
  
  for (JsonObject obj : array) {
    if (stationCount >= MAX_STATIONS) break;
    
    ChargingStation station = {};
    station.id = obj["id"];
    station.displayName = obj["displayName"].as<String>();
    station.technicalName = obj["technicalName"].as<String>();
    station.type = obj["type"].as<String>();
    station.status = obj["status"].as<String>();
    station.description = obj["description"].as<String>();
    station.ipAddress = obj["ipAddress"].as<String>();
    station.maxPower = obj["maxPower"];
    station.currentPower = obj["currentPower"];
    station.chargerPower = obj["chargerPower"];
    station.masterAvailablePower = obj["masterAvailablePower"];
    
    station.voltagePhase1 = obj["voltagePhase1"];
    station.voltagePhase2 = obj["voltagePhase2"];
    station.voltagePhase3 = obj["voltagePhase3"];
    station.currentPhase1 = obj["currentPhase1"];
    station.currentPhase2 = obj["currentPhase2"];
    station.currentPhase3 = obj["currentPhase3"];
    
    station.carConnection = obj["carConnection"];
    station.carChargingPermission = obj["carChargingPermission"];
    station.carError = obj["carError"];
    station.masterOnline = obj["masterOnline"];
    station.masterChargingPermission = obj["masterChargingPermission"];
    
    station.singlePhaseConnection = obj["singlePhaseConnection"];
    station.powerOverconsumption = obj["powerOverconsumption"];
    station.fixedPower = obj["fixedPower"];
    
    station.lastUpdate = millis();
    
    stations[stationCount] = station;
    stationCount++;
  }
  
  logMessage("INFO", "Загружено " + String(stationCount) + " станций");
}

void saveStations() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  JsonArray array = doc.to<JsonArray>();
  
  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.createNestedObject();
    populateStationJson(station, stations[i]);
  }
  
  File file = LittleFS.open("/stations.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    logMessage("DEBUG", "Данные станций сохранены");
  } else {
    logMessage("ERROR", "Ошибка сохранения данных станций");
  }
}

void createDefaultConfiguration() {
  DynamicJsonDocument doc(1024);
  doc["version"] = "1.0";
  doc["maxStations"] = MAX_STATIONS;
  doc["websocketPort"] = 81;
  doc["updateInterval"] = WEBSOCKET_UPDATE_INTERVAL;
  doc["logLevel"] = "INFO";
  
  File file = LittleFS.open("/config.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    logMessage("INFO", "Создана стандартная конфигурация");
  }
}

void createDefaultStations() {
  stationCount = 3;
  
  stations[0] = {1, "ESP32 Master Station", "ESP32-MASTER-001", "master", "online", "Главная станция управления", WiFi.localIP().toString(), 50.0, 25.5, 25.5, 40.0, 230.0, 230.0, 230.0, 15.2, 18.7, 16.3, true, true, false, true, true, false, false, false, millis()};
  
  stations[1] = {2, "Fast Charging Point 1", "ESP32-SLAVE-001", "slave", "charging", "Быстрая зарядка тип 1", "192.168.1.101", 22.0, 18.5, 18.5, 40.0, 230.0, 230.0, 230.0, 8.2, 8.1, 8.3, true, true, false, true, true, false, false, true, millis()};
  
  stations[2] = {3, "Standard Charging Point", "ESP32-SLAVE-002", "slave", "available", "Стандартная зарядка", "192.168.1.102", 11.0, 0.0, 0.0, 40.0, 230.0, 0.0, 0.0, 0.0, 0.0, 0.0, false, false, false, true, false, true, false, false, millis()};
  
  saveStations();
  logMessage("INFO", "Созданы тестовые станции");
}

int getNextStationId() {
  int maxId = 0;
  for (int i = 0; i < stationCount; i++) {
    if (stations[i].id > maxId) {
      maxId = stations[i].id;
    }
  }
  return maxId + 1;
}

// WebSocket функции

void sendStationsToClient(uint8_t clientNum) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "stations_update";
  JsonArray array = doc.createNestedArray("data");
  
  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.createNestedObject();
    populateStationJson(station, stations[i]);
  }
  
  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(clientNum, message);
}

void broadcastStationUpdates() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "stations_update";
  JsonArray array = doc.createNestedArray("data");
  
  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.createNestedObject();
    populateStationJson(station, stations[i]);
  }
  
  String message;
  serializeJson(doc, message);
  webSocket.broadcastTXT(message);
}

void broadcastStationUpdate(const ChargingStation& station) {
  DynamicJsonDocument doc(2048);
  doc["type"] = "station_update";
  JsonObject stationObj = doc.createNestedObject("data");
  populateStationJson(stationObj, station);
  
  String message;
  serializeJson(doc, message);
  webSocket.broadcastTXT(message);
}

void broadcastStationDeleted(int stationId) {
  DynamicJsonDocument doc(256);
  doc["type"] = "station_deleted";
  doc["id"] = stationId;
  
  String message;
  serializeJson(doc, message);
  webSocket.broadcastTXT(message);
}

void handleWebSocketMessage(uint8_t clientNum, const char* payload) {
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, payload) != DeserializationError::Ok) {
    return;
  }
  
  String type = doc["type"];
  
  if (type == "ping") {
    DynamicJsonDocument response(128);
    response["type"] = "pong";
    response["timestamp"] = millis();
    
    String message;
    serializeJson(response, message);
    webSocket.sendTXT(clientNum, message);
  }
  else if (type == "get_stations") {
    sendStationsToClient(clientNum);
  }
  else if (type == "get_system_info") {
    // Отправляем системную информацию
    DynamicJsonDocument response(1024);
    response["type"] = "system_info";
    response["freeHeap"] = ESP.getFreeHeap();
    response["uptime"] = millis();
    response["stationCount"] = stationCount;
    
    String message;
    serializeJson(response, message);
    webSocket.sendTXT(clientNum, message);
  }
}

// Системные функции

void logMessage(String level, String message) {
  systemLogs[logIndex] = {millis(), level, message};
  logIndex = (logIndex + 1) % MAX_LOG_ENTRIES;
  
  // Также выводим в Serial
  Serial.println("[" + level + "] " + message);
}

void checkMemoryUsage() {
  size_t freeHeap = ESP.getFreeHeap();
  size_t freePsram = ESP.getFreePsram();
  
  if (freeHeap < 50000) { // Менее 50KB свободной heap памяти
    logMessage("WARNING", "Низкий уровень heap памяти: " + String(freeHeap) + " bytes");
  }
  
  if (freePsram < 100000) { // Менее 100KB свободной PSRAM
    logMessage("WARNING", "Низкий уровень PSRAM: " + String(freePsram) + " bytes");
  }
  
  logMessage("DEBUG", "Память: Heap=" + String(freeHeap) + ", PSRAM=" + String(freePsram));
}