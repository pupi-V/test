#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <time.h>

// Настройки WiFi сети для подключения
const char* ssid = "ВАШ_WIFI_SSID";        // Замените на имя вашей WiFi сети
const char* password = "ВАШ_WIFI_ПАРОЛЬ";  // Замените на пароль вашей WiFi сети

// Веб-сервер и WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Структура зарядной станции
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

// Функции работы с JSON
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

void jsonToStation(const JsonObject& json, ChargingStation& station) {
  if (json["displayName"]) station.displayName = json["displayName"].as<String>();
  if (json["technicalName"]) station.technicalName = json["technicalName"].as<String>();
  if (json["type"]) station.type = json["type"].as<String>();
  if (json["status"]) station.status = json["status"].as<String>();
  if (json["maxPower"]) station.maxPower = json["maxPower"];
  if (json["currentPower"]) station.currentPower = json["currentPower"];
  if (json["availablePower"]) station.availablePower = json["availablePower"];
  if (json["carConnected"]) station.carConnected = json["carConnected"];
  if (json["chargingAllowed"]) station.chargingAllowed = json["chargingAllowed"];
  if (json["hasError"]) station.hasError = json["hasError"];
  if (json["errorMessage"]) station.errorMessage = json["errorMessage"].as<String>();
  if (json["masterId"]) station.masterId = json["masterId"];
  if (json["voltageL1"]) station.voltageL1 = json["voltageL1"];
  if (json["voltageL2"]) station.voltageL2 = json["voltageL2"];
  if (json["voltageL3"]) station.voltageL3 = json["voltageL3"];
  if (json["currentL1"]) station.currentL1 = json["currentL1"];
  if (json["currentL2"]) station.currentL2 = json["currentL2"];
  if (json["currentL3"]) station.currentL3 = json["currentL3"];
}

void saveStationsToFile() {
  File file = SPIFFS.open("/stations.json", "w");
  if (!file) {
    Serial.println("Ошибка создания файла stations.json");
    return;
  }

  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.add<JsonObject>();
    stationToJson(stations[i], station);
  }

  serializeJson(doc, file);
  file.close();
  Serial.println("Данные станций сохранены в файл");
}

String getCurrentTime() {
  time_t now;
  time(&now);
  return String(ctime(&now));
}

void createTestStations() {
  if (stationCount == 0) {
    stationCount = 2;

    // Станция 1
    stations[0].id = 1;
    stations[0].displayName = "Станция A1";
    stations[0].technicalName = "ST_A1_001";
    stations[0].type = "master";
    stations[0].status = "available";
    stations[0].maxPower = 22.0;
    stations[0].currentPower = 0.0;
    stations[0].availablePower = 22.0;
    stations[0].carConnected = false;
    stations[0].chargingAllowed = true;
    stations[0].hasError = false;
    stations[0].errorMessage = "";
    stations[0].masterId = 0;
    stations[0].voltageL1 = 230.0;
    stations[0].voltageL2 = 230.0;
    stations[0].voltageL3 = 230.0;
    stations[0].currentL1 = 0.0;
    stations[0].currentL2 = 0.0;
    stations[0].currentL3 = 0.0;
    stations[0].lastUpdate = getCurrentTime();

    // Станция 2
    stations[1].id = 2;
    stations[1].displayName = "Станция B2";
    stations[1].technicalName = "ST_B2_002";
    stations[1].type = "slave";
    stations[1].status = "charging";
    stations[1].maxPower = 11.0;
    stations[1].currentPower = 7.5;
    stations[1].availablePower = 3.5;
    stations[1].carConnected = true;
    stations[1].chargingAllowed = true;
    stations[1].hasError = false;
    stations[1].errorMessage = "";
    stations[1].masterId = 1;
    stations[1].voltageL1 = 230.0;
    stations[1].voltageL2 = 230.0;
    stations[1].voltageL3 = 230.0;
    stations[1].currentL1 = 10.9;
    stations[1].currentL2 = 10.9;
    stations[1].currentL3 = 10.9;
    stations[1].lastUpdate = getCurrentTime();

    saveStationsToFile();
    Serial.println("Созданы тестовые станции");
  }
}

void updateStationFromJson(ChargingStation& station, const JsonObject& json) {
  jsonToStation(json, station);
  station.lastUpdate = getCurrentTime();
}

int findStationIndex(int id) {
  for (int i = 0; i < stationCount; i++) {
    if (stations[i].id == id) {
      return i;
    }
  }
  return -1;
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

void loadStationsFromFile() {
  if (!SPIFFS.exists("/stations.json")) {
    Serial.println("Файл stations.json не найден, создаем тестовые данные");
    createTestStations();
    return;
  }

  File file = SPIFFS.open("/stations.json", "r");
  if (!file) {
    Serial.println("Ошибка открытия файла stations.json");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("Ошибка парсинга JSON файла");
    return;
  }

  JsonArray array = doc.as<JsonArray>();
  stationCount = 0;

  for (JsonVariant v : array) {
    if (stationCount >= 50) break;
    JsonObject obj = v.as<JsonObject>();
    stations[stationCount].id = obj["id"];
    jsonToStation(obj, stations[stationCount]);
    stationCount++;
  }

  Serial.printf("Загружено %d станций из файла\n", stationCount);
}

void updateStationsData() {
  // Симуляция обновления данных
  for (int i = 0; i < stationCount; i++) {
    if (stations[i].status == "charging") {
      // Небольшие изменения тока
      stations[i].currentL1 += random(-50, 50) / 100.0;
      stations[i].currentL2 += random(-50, 50) / 100.0;
      stations[i].currentL3 += random(-50, 50) / 100.0;

      // Ограничиваем значения
      stations[i].currentL1 = max(0.0f, min(16.0f, stations[i].currentL1));
      stations[i].currentL2 = max(0.0f, min(16.0f, stations[i].currentL2));
      stations[i].currentL3 = max(0.0f, min(16.0f, stations[i].currentL3));
    }
    stations[i].lastUpdate = getCurrentTime();
  }
}

void broadcastStationsUpdate() {
  JsonDocument doc;
  doc["type"] = "stations_update";
  JsonArray array = doc["data"].to<JsonArray>();

  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.add<JsonObject>();
    stationToJson(stations[i], station);
  }

  String message;
  serializeJson(doc, message);
  ws.textAll(message);
}

void sendStationsToClient(AsyncWebSocketClient* client) {
  JsonDocument doc;
  doc["type"] = "stations_data";
  JsonArray array = doc["data"].to<JsonArray>();

  for (int i = 0; i < stationCount; i++) {
    JsonObject station = array.add<JsonObject>();
    stationToJson(stations[i], station);
  }

  String message;
  serializeJson(doc, message);
  client->text(message);
}

void handleWebSocketMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) return;

  String action = doc["action"];
  if (action == "update_station") {
    int stationId = doc["stationId"];
    int stationIndex = findStationIndex(stationId);
    if (stationIndex >= 0) {
      updateStationFromJson(stations[stationIndex], doc["data"]);
      saveStationsToFile();
      broadcastStationsUpdate();
    }
  }
}

void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket клиент #%u подключен с IP %s\n", client->id(), client->remoteIP().toString().c_str());
      sendStationsToClient(client);
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket клиент #%u отключен\n", client->id());
      break;

    case WS_EVT_DATA:
      Serial.printf("WebSocket получено %u байт данных\n", len);
      handleWebSocketMessage(client, data, len);
      break;

    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      Serial.println("WebSocket ошибка или pong");
      break;
  }
}

void setupAPIRoutes() {
  // Настройка CORS для всех запросов
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

  // OPTIONS обработчик для CORS
  server.on("/api/stations", HTTP_OPTIONS, [](AsyncWebServerRequest* request) {
    Serial.println("API: OPTIONS /api/stations");
    request->send(200);
  });

  // GET /api/stations
  server.on("/api/stations", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("API: GET /api/stations");
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < stationCount; i++) {
      JsonObject station = array.add<JsonObject>();
      stationToJson(stations[i], station);
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // POST /api/stations
  server.on("/api/stations", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      Serial.printf("API: POST /api/stations (получено %d байт)\n", len);
      if (stationCount >= 50) {
        request->send(400, "application/json", "{\"error\":\"Максимальное количество станций достигнуто\"}");
        return;
      }

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"error\":\"Неверный JSON\"}");
        return;
      }

      ChargingStation newStation;
      JsonObject obj = doc.as<JsonObject>();
      jsonToStation(obj, newStation);
      newStation.id = getNextStationId();
      stations[stationCount] = newStation;
      stationCount++;

      saveStationsToFile();

      JsonDocument responseDoc;
      JsonObject responseObj = responseDoc.to<JsonObject>();
      stationToJson(newStation, responseObj);

      String response;
      serializeJson(responseDoc, response);
      request->send(201, "application/json", response);
    });

  // PATCH /api/stations/:id
  server.on("^\\/api\\/stations\\/([0-9]+)$", HTTP_PATCH, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      String idStr = request->pathArg(0);
      int stationId = idStr.toInt();
      Serial.printf("API: PATCH /api/stations/%d (получено %d байт)\n", stationId, len);
      int stationIndex = findStationIndex(stationId);

      if (stationIndex < 0) {
        request->send(404, "application/json", "{\"error\":\"Станция не найдена\"}");
        return;
      }

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"error\":\"Неверный JSON\"}");
        return;
      }

      JsonObject obj = doc.as<JsonObject>();
      updateStationFromJson(stations[stationIndex], obj);
      saveStationsToFile();

      JsonDocument responseDoc;
      JsonObject responseObj = responseDoc.to<JsonObject>();
      stationToJson(stations[stationIndex], responseObj);

      String response;
      serializeJson(responseDoc, response);
      request->send(200, "application/json", response);
    });

  // DELETE /api/stations/:id
  server.on("^\\/api\\/stations\\/([0-9]+)$", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String idStr = request->pathArg(0);
    int stationId = idStr.toInt();
    Serial.printf("API: DELETE /api/stations/%d\n", stationId);
    int stationIndex = findStationIndex(stationId);

    if (stationIndex < 0) {
      request->send(404, "application/json", "{\"error\":\"Станция не найдена\"}");
      return;
    }

    // Сдвигаем все элементы влево
    for (int i = stationIndex; i < stationCount - 1; i++) {
      stations[i] = stations[i + 1];
    }
    stationCount--;

    saveStationsToFile();
    request->send(200, "application/json", "{\"message\":\"Станция удалена\"}");
  });

  // POST /api/esp32/scan
  server.on("/api/esp32/scan", HTTP_POST, [](AsyncWebServerRequest* request) {
    Serial.println("API: POST /api/esp32/scan");
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    // Добавляем текущую плату в результат
    JsonObject board = array.add<JsonObject>();
    board["id"] = "esp32_local";
    board["type"] = "ESP32";
    board["ip"] = WiFi.softAPIP().toString();
    board["name"] = "Локальная ESP32";
    board["status"] = "online";
    board["lastSeen"] = getCurrentTime();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Charging Station Management System ===");

  // Инициализация файловой системы
  if (!SPIFFS.begin(true)) {
    Serial.println("ОШИБКА: Не удалось инициализировать SPIFFS");
    return;
  }
  Serial.println("✓ SPIFFS инициализирована");

  // Загрузка данных станций из файла
  loadStationsFromFile();

  // Подключение к существующей WiFi сети
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Подключение к WiFi сети: ");
  Serial.println(ssid);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi подключен успешно");
    Serial.print("IP адрес: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Не удалось подключиться к WiFi");
    Serial.println("Переключение в режим точки доступа...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_ChargingStations", "12345678");
    Serial.print("IP адрес точки доступа: ");
    Serial.println(WiFi.softAPIP());
  }

  // Настройка времени
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("✓ Настройка времени завершена");

  // Настройка mDNS для удобного доступа
  if (MDNS.begin("chargingstations")) {
    Serial.println("✓ mDNS запущен: http://chargingstations.local");
  }

  // Настройка WebSocket
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  // API маршруты
  setupAPIRoutes();

  // Проверка наличия веб-файлов
  if (!SPIFFS.exists("/www/index.html")) {
    Serial.println("ВНИМАНИЕ: /www/index.html не найден, создаем базовую страницу");
    File file = SPIFFS.open("/index.html", "w");
    if (file) {
      file.print(R"(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><title>ESP32 Charging Stations</title></head>
<body><h1>ESP32 Charging Station System</h1>
<p>Система управления зарядными станциями работает!</p>
<p>API доступно по адресу: <a href="/api/stations">/api/stations</a></p>
<script>
fetch('/api/stations')
  .then(r => r.json())
  .then(data => {
    document.body.innerHTML += '<pre>' + JSON.stringify(data, null, 2) + '</pre>';
  })
  .catch(e => {
    document.body.innerHTML += '<p style="color:red">Ошибка API: ' + e + '</p>';
  });
</script></body></html>)");
      file.close();
      Serial.println("✓ Создана базовая веб-страница");
    }
  }

  // Статические файлы веб-интерфейса
  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  // Главная страница
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Запрос главной страницы");
    if (SPIFFS.exists("/www/index.html")) {
      request->send(SPIFFS, "/www/index.html", "text/html");
    } else if (SPIFFS.exists("/index.html")) {
      request->send(SPIFFS, "/index.html", "text/html");
    } else {
      request->send(200, "text/html", 
        "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32</title></head>"
        "<body><h1>ESP32 Charging Station System</h1>"
        "<p>Система работает! API: <a href='/api/stations'>/api/stations</a></p></body></html>");
    }
  });

  // Обработчик для всех неизвестных запросов
  server.onNotFound([](AsyncWebServerRequest *request) {
    String url = request->url();
    String method = request->methodToString();
    Serial.printf("404: %s %s\n", method.c_str(), url.c_str());
    
    if (url.startsWith("/api/")) {
      request->send(404, "application/json", "{\"error\":\"API endpoint not found\"}");
    } else {
      request->send(404, "text/html", 
        "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>404</title></head>"
        "<body><h1>404 - Страница не найдена</h1>"
        "<p><a href='/'>Главная страница</a> | <a href='/api/stations'>API станций</a></p></body></html>");
    }
  });

  // Запуск веб-сервера
  server.begin();
  Serial.println("✓ Веб-сервер запущен на порту 80");
  Serial.println("📡 Доступ к системе:");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("   WiFi сеть: %s\n", ssid);
    Serial.printf("   IP адрес: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.printf("   mDNS: http://chargingstations.local\n");
    Serial.printf("   API: http://%s/api/stations\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.printf("   WiFi сеть: ESP32_ChargingStations\n");
    Serial.printf("   Пароль: 12345678\n");
    Serial.printf("   IP адрес: http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("   API: http://%s/api/stations\n", WiFi.softAPIP().toString().c_str());
  }

  // Создание тестовых станций если файл не найден
  if (stationCount == 0) {
    createTestStations();
  }

  Serial.println("=== Система готова к работе ===");
}

void loop() {
  // Обновление данных каждые 5 секунд
  if (millis() - lastUpdate > updateInterval) {
    updateStationsData();
    broadcastStationsUpdate();
    lastUpdate = millis();
    Serial.printf("Данные обновлены. Подключенных клиентов: %u\n", ws.count());
  }

  // Обработка WebSocket соединений
  ws.cleanupClients();

  delay(100);
}