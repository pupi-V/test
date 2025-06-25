/*
 * ESP32 Система управления зарядными станциями для электромобилей
 * Версия для модулей с 16MB Flash памятью
 * 
 * Поддерживаемые функции:
 * - Полнофункциональный веб-интерфейс на React
 * - WebSocket для real-time обновлений
 * - Управление до 50 зарядными станциями
 * - OTA обновления
 * - LittleFS файловая система
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AsyncWebSocket.h>
#include <ESPmDNS.h>

// Настройки WiFi сети
const char* ssid = "ESP32_ChargingStations";
const char* password = "12345678";

// Веб-сервер и WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Структура данных зарядной станции
struct ChargingStation {
  int id;
  String displayName;
  String technicalName;
  String type;
  String status;
  float maxPower;
  float currentPower;
  float availablePower;
  bool carConnected;
  bool chargingAllowed;
  bool hasError;
  String errorMessage;
  int masterId;
  float voltageL1;
  float voltageL2;
  float voltageL3;
  float currentL1;
  float currentL2;
  float currentL3;
  String lastUpdate;
};

// Массив станций (максимум 50 для 16MB модуля)
ChargingStation stations[50];
int stationCount = 0;

// Таймер для обновления данных
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 5000; // 5 секунд

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Charging Station Management System ===");
  
  // Инициализация файловой системы
  if (!LittleFS.begin(true)) {
    Serial.println("ОШИБКА: Не удалось инициализировать LittleFS");
    return;
  }
  Serial.println("✓ LittleFS инициализирована");
  
  // Загрузка данных станций из файла
  loadStationsFromFile();
  
  // Настройка WiFi точки доступа
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  
  Serial.println("✓ WiFi точка доступа создана");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.softAPIP());
  
  // Настройка mDNS для удобного доступа
  if (MDNS.begin("chargingstations")) {
    Serial.println("✓ mDNS запущен: http://chargingstations.local");
  }
  
  // Настройка WebSocket
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  
  // API маршруты
  setupAPIRoutes();
  
  // Статические файлы веб-интерфейса
  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");
  
  // Обслуживание отдельных файлов с правильными MIME типами
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/www/index.html", "text/html");
  });
  
  // Обработка 404 ошибок
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Страница не найдена");
  });
  
  // Запуск веб-сервера
  server.begin();
  Serial.println("✓ Веб-сервер запущен на порту 80");
  
  // Создание тестовых данных станций
  createTestStations();
  
  Serial.println("=== Система готова к работе ===");
}

void loop() {
  // Обновление данных станций каждые 5 секунд
  if (millis() - lastUpdate > updateInterval) {
    updateStationsData();
    broadcastStationsUpdate();
    lastUpdate = millis();
  }
  
  // Очистка WebSocket соединений
  ws.cleanupClients();
  
  delay(10);
}

// Настройка API маршрутов
void setupAPIRoutes() {
  // GET /api/stations - получить все станции
  server.on("/api/stations", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc(8192);
    JsonArray array = doc.to<JsonArray>();
    
    for (int i = 0; i < stationCount; i++) {
      JsonObject station = array.createNestedObject();
      stationToJson(stations[i], station);
    }
    
    serializeJson(doc, *response);
    request->send(response);
  });
  
  // POST /api/stations - создать новую станцию
  server.on("/api/stations", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, (char*)data);
      
      if (stationCount < 50) {
        ChargingStation newStation;
        jsonToStation(doc, newStation);
        newStation.id = getNextStationId();
        stations[stationCount++] = newStation;
        
        saveStationsToFile();
        
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument responseDoc(1024);
        stationToJson(newStation, responseDoc);
        serializeJson(responseDoc, *response);
        request->send(response);
      } else {
        request->send(400, "application/json", "{\"error\":\"Максимум 50 станций\"}");
      }
    });
  
  // PATCH /api/stations/:id - обновить станцию
  server.on("/api/stations", HTTP_PATCH, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String path = request->url();
      int stationId = path.substring(path.lastIndexOf('/') + 1).toInt();
      
      int stationIndex = findStationIndex(stationId);
      if (stationIndex == -1) {
        request->send(404, "application/json", "{\"error\":\"Станция не найдена\"}");
        return;
      }
      
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, (char*)data);
      
      updateStationFromJson(stations[stationIndex], doc);
      saveStationsToFile();
      
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      DynamicJsonDocument responseDoc(1024);
      stationToJson(stations[stationIndex], responseDoc);
      serializeJson(responseDoc, *response);
      request->send(response);
    });
  
  // DELETE /api/stations/:id - удалить станцию
  server.on("/api/stations", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    String path = request->url();
    int stationId = path.substring(path.lastIndexOf('/') + 1).toInt();
    
    int stationIndex = findStationIndex(stationId);
    if (stationIndex == -1) {
      request->send(404, "application/json", "{\"error\":\"Станция не найдена\"}");
      return;
    }
    
    // Сдвигаем массив
    for (int i = stationIndex; i < stationCount - 1; i++) {
      stations[i] = stations[i + 1];
    }
    stationCount--;
    
    saveStationsToFile();
    request->send(200, "application/json", "{\"success\":true}");
  });
  
  // GET /api/esp32/scan - сканирование ESP32 устройств
  server.on("/api/esp32/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    
    JsonObject board = array.createNestedObject();
    board["id"] = "esp32-master";
    board["type"] = "master";
    board["ip"] = WiFi.softAPIP().toString();
    board["name"] = "ESP32 Master Station";
    board["status"] = "online";
    board["lastSeen"] = "сейчас";
    
    serializeJson(doc, *response);
    request->send(response);
  });
}

// Обработка WebSocket событий
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                     AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket клиент #%u подключен с IP %s\n", 
                    client->id(), client->remoteIP().toString().c_str());
      // Отправляем текущие данные новому клиенту
      sendStationsToClient(client);
      break;
      
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket клиент #%u отключен\n", client->id());
      break;
      
    case WS_EVT_ERROR:
      Serial.printf("WebSocket ошибка клиента #%u: %s\n", client->id(), (char*)data);
      break;
      
    case WS_EVT_DATA:
      // Обработка входящих данных от клиента
      handleWebSocketMessage(client, data, len);
      break;
  }
}

// Обработка WebSocket сообщений
void handleWebSocketMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, (char*)data);
  
  String action = doc["action"];
  
  if (action == "updateStation") {
    int stationId = doc["stationId"];
    int stationIndex = findStationIndex(stationId);
    
    if (stationIndex != -1) {
      updateStationFromJson(stations[stationIndex], doc["data"]);
      saveStationsToFile();
      broadcastStationsUpdate();
    }
  }
}

// Отправка данных станций всем WebSocket клиентам
void broadcastStationsUpdate() {
  DynamicJsonDocument doc(8192);
  doc["type"] = "stationsUpdate";
  JsonArray array = doc.createNestedArray("data");
  
  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.createNestedObject();
    stationToJson(stations[i], station);
  }
  
  String message;
  serializeJson(doc, message);
  ws.textAll(message);
}

// Отправка данных станций конкретному клиенту
void sendStationsToClient(AsyncWebSocketClient *client) {
  DynamicJsonDocument doc(8192);
  doc["type"] = "stationsUpdate";
  JsonArray array = doc.createNestedArray("data");
  
  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.createNestedObject();
    stationToJson(stations[i], station);
  }
  
  String message;
  serializeJson(doc, message);
  client->text(message);
}

// Преобразование структуры станции в JSON
void stationToJson(const ChargingStation& station, JsonObject& json) {
  json["id"] = station.id;
  json["displayName"] = station.displayName;
  json["technicalName"] = station.technicalName;
  json["type"] = station.type;
  json["status"] = station.status;
  json["maxPower"] = station.maxPower;
  json["currentPower"] = station.currentPower;
  json["availablePower"] = station.availablePower;
  json["carConnected"] = station.carConnected;
  json["chargingAllowed"] = station.chargingAllowed;
  json["hasError"] = station.hasError;
  json["errorMessage"] = station.errorMessage;
  json["masterId"] = station.masterId;
  json["voltageL1"] = station.voltageL1;
  json["voltageL2"] = station.voltageL2;
  json["voltageL3"] = station.voltageL3;
  json["currentL1"] = station.currentL1;
  json["currentL2"] = station.currentL2;
  json["currentL3"] = station.currentL3;
  json["lastUpdate"] = station.lastUpdate;
}

// Преобразование JSON в структуру станции
void jsonToStation(const JsonObject& json, ChargingStation& station) {
  if (json.containsKey("displayName")) station.displayName = json["displayName"].as<String>();
  if (json.containsKey("technicalName")) station.technicalName = json["technicalName"].as<String>();
  if (json.containsKey("type")) station.type = json["type"].as<String>();
  if (json.containsKey("status")) station.status = json["status"].as<String>();
  if (json.containsKey("maxPower")) station.maxPower = json["maxPower"];
  if (json.containsKey("currentPower")) station.currentPower = json["currentPower"];
  if (json.containsKey("availablePower")) station.availablePower = json["availablePower"];
  if (json.containsKey("carConnected")) station.carConnected = json["carConnected"];
  if (json.containsKey("chargingAllowed")) station.chargingAllowed = json["chargingAllowed"];
  if (json.containsKey("hasError")) station.hasError = json["hasError"];
  if (json.containsKey("errorMessage")) station.errorMessage = json["errorMessage"].as<String>();
  if (json.containsKey("masterId")) station.masterId = json["masterId"];
  if (json.containsKey("voltageL1")) station.voltageL1 = json["voltageL1"];
  if (json.containsKey("voltageL2")) station.voltageL2 = json["voltageL2"];
  if (json.containsKey("voltageL3")) station.voltageL3 = json["voltageL3"];
  if (json.containsKey("currentL1")) station.currentL1 = json["currentL1"];
  if (json.containsKey("currentL2")) station.currentL2 = json["currentL2"];
  if (json.containsKey("currentL3")) station.currentL3 = json["currentL3"];
}

// Обновление станции из JSON (частичное обновление)
void updateStationFromJson(ChargingStation& station, const JsonObject& json) {
  jsonToStation(json, station);
  station.lastUpdate = getCurrentTime();
}

// Поиск индекса станции по ID
int findStationIndex(int id) {
  for (int i = 0; i < stationCount; i++) {
    if (stations[i].id == id) return i;
  }
  return -1;
}

// Получение следующего ID для новой станции
int getNextStationId() {
  int maxId = 0;
  for (int i = 0; i < stationCount; i++) {
    if (stations[i].id > maxId) maxId = stations[i].id;
  }
  return maxId + 1;
}

// Получение текущего времени в виде строки
String getCurrentTime() {
  return String(millis() / 1000) + "s";
}

// Создание тестовых станций
void createTestStations() {
  if (stationCount == 0) {
    // Станция 1 - Master
    stations[0] = {
      1, "Главная станция", "MASTER_01", "master", "available",
      50.0, 0.0, 50.0, false, true, false, "",
      0, 230.5, 231.2, 229.8, 0.0, 0.0, 0.0, getCurrentTime()
    };
    
    // Станция 2 - Slave в работе
    stations[1] = {
      2, "Станция №2", "SLAVE_02", "slave", "charging",
      22.0, 18.5, 3.5, true, true, false, "",
      1, 229.1, 230.8, 231.5, 25.2, 24.8, 26.1, getCurrentTime()
    };
    
    // Станция 3 - Slave с ошибкой
    stations[2] = {
      3, "Станция №3", "SLAVE_03", "slave", "error",
      22.0, 0.0, 0.0, false, false, true, "Ошибка связи с контроллером",
      1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, getCurrentTime()
    };
    
    stationCount = 3;
    saveStationsToFile();
    Serial.println("✓ Созданы тестовые станции");
  }
}

// Обновление данных станций (симуляция реальных данных)
void updateStationsData() {
  for (int i = 0; i < stationCount; i++) {
    ChargingStation& station = stations[i];
    
    // Симуляция изменения данных
    if (station.status == "charging") {
      // Небольшие колебания тока и мощности
      station.currentPower += (random(-200, 200) / 100.0);
      if (station.currentPower < 0) station.currentPower = 0;
      if (station.currentPower > station.maxPower) station.currentPower = station.maxPower;
      
      station.availablePower = station.maxPower - station.currentPower;
      
      // Обновление токов по фазам
      float baseCurrent = station.currentPower / (3 * 230) * 1000; // мА
      station.currentL1 = baseCurrent + random(-500, 500) / 100.0;
      station.currentL2 = baseCurrent + random(-500, 500) / 100.0;
      station.currentL3 = baseCurrent + random(-500, 500) / 100.0;
    }
    
    // Обновление напряжений (небольшие колебания)
    if (!station.hasError) {
      station.voltageL1 = 230.0 + random(-20, 20) / 10.0;
      station.voltageL2 = 230.0 + random(-20, 20) / 10.0;
      station.voltageL3 = 230.0 + random(-20, 20) / 10.0;
    }
    
    station.lastUpdate = getCurrentTime();
  }
}

// Сохранение данных станций в файл
void saveStationsToFile() {
  File file = LittleFS.open("/stations.json", "w");
  if (!file) {
    Serial.println("ОШИБКА: Не удалось открыть файл для записи");
    return;
  }
  
  DynamicJsonDocument doc(8192);
  JsonArray array = doc.to<JsonArray>();
  
  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.createNestedObject();
    stationToJson(stations[i], station);
  }
  
  serializeJson(doc, file);
  file.close();
}

// Загрузка данных станций из файла
void loadStationsFromFile() {
  if (!LittleFS.exists("/stations.json")) {
    Serial.println("Файл stations.json не найден, будут созданы тестовые данные");
    return;
  }
  
  File file = LittleFS.open("/stations.json", "r");
  if (!file) {
    Serial.println("ОШИБКА: Не удалось открыть файл для чтения");
    return;
  }
  
  DynamicJsonDocument doc(8192);
  deserializeJson(doc, file);
  file.close();
  
  JsonArray array = doc.as<JsonArray>();
  stationCount = 0;
  
  for (JsonObject stationObj : array) {
    if (stationCount < 50) {
      stations[stationCount].id = stationObj["id"];
      jsonToStation(stationObj, stations[stationCount]);
      stationCount++;
    }
  }
  
  Serial.printf("✓ Загружено %d станций из файла\n", stationCount);
}