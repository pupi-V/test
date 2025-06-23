#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

// WiFi настройки - ИЗМЕНИТЕ НА СВОИ
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* hostname = "charging-station";

WebServer server(80);

// Структура данных станции
struct StationData {
  String displayName = "ESP32 Station";
  String technicalName = "ESP32-001";
  String type = "slave";
  float maxPower = 22.0;
  float currentPower = 0.0;
  String status = "available";
  String ipAddress = "";
  
  // Car секция
  bool carConnection = false;
  bool carChargingPermission = false;
  bool carError = false;
  
  // Master секция
  bool masterOnline = false;
  bool masterChargingPermission = false;
  float masterAvailablePower = 0.0;
  
  // Параметры зарядки
  float voltagePhase1 = 230.0;
  float voltagePhase2 = 230.0;
  float voltagePhase3 = 230.0;
  float currentPhase1 = 0.0;
  float currentPhase2 = 0.0;
  float currentPhase3 = 0.0;
  
  // Статус зарядного устройства
  bool chargerOnline = true;
  bool chargerError = false;
  bool powerOverconsumption = false;
  bool fixedPower = false;
};

StationData station;

// Минифицированный HTML интерфейс
const char* WEB_PAGE = R"rawliteral(<!DOCTYPE html><html lang="ru"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>ESP32 Charging Station</title><style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:Arial,sans-serif;background:#f5f5f5;padding:10px}.container{max-width:600px;margin:0 auto;background:white;padding:15px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1)}.header{text-align:center;margin-bottom:20px;color:#333}h1{font-size:1.5em;margin-bottom:5px}.section{margin-bottom:20px;padding:15px;border:1px solid #ddd;border-radius:6px}.section h3{margin-bottom:10px;color:#2c5282;font-size:1.1em}.field{display:flex;justify-content:space-between;align-items:center;padding:5px 0;border-bottom:1px solid #eee}.field:last-child{border-bottom:none}.label{font-weight:bold;color:#555;font-size:0.9em}.value{color:#333;font-size:0.9em}.status{padding:3px 6px;border-radius:3px;color:white;font-size:0.8em}.status.available{background:#48bb78}.status.charging{background:#3182ce}.status.offline{background:#e53e3e}.checkbox{width:16px;height:16px}.refresh-btn{background:#3182ce;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;margin:8px 0;font-size:0.9em}.refresh-btn:hover{background:#2c5282}@media (max-width:600px){.container{padding:10px}.field{flex-direction:column;align-items:flex-start}.value{margin-top:2px}}</style></head><body><div class="container"><div class="header"><h1>🔌 ESP32 Charging Station</h1><p id="station-name">Станция загружается...</p><button class="refresh-btn" onclick="loadData()">Обновить</button></div><div class="section"><h3>Основная информация</h3><div class="field"><span class="label">Название:</span><span class="value" id="displayName">-</span></div><div class="field"><span class="label">Тип:</span><span class="value" id="type">-</span></div><div class="field"><span class="label">Статус:</span><span class="status" id="status">-</span></div><div class="field"><span class="label">Макс. мощность:</span><span class="value" id="maxPower">-</span></div><div class="field"><span class="label">Текущая мощность:</span><span class="value" id="currentPower">-</span></div><div class="field"><span class="label">IP адрес:</span><span class="value" id="ipAddress">-</span></div></div><div class="section"><h3>Автомобиль</h3><div class="field"><span class="label">Подключен:</span><input type="checkbox" class="checkbox" id="carConnection" disabled></div><div class="field"><span class="label">Разрешение зарядки:</span><input type="checkbox" class="checkbox" id="carChargingPermission" disabled></div><div class="field"><span class="label">Ошибка:</span><input type="checkbox" class="checkbox" id="carError" disabled></div></div><div class="section"><h3>Master плата</h3><div class="field"><span class="label">Онлайн:</span><input type="checkbox" class="checkbox" id="masterOnline" disabled></div><div class="field"><span class="label">Разрешение зарядки:</span><input type="checkbox" class="checkbox" id="masterChargingPermission" disabled></div><div class="field"><span class="label">Доступная мощность:</span><span class="value" id="masterAvailablePower">-</span></div></div><div class="section"><h3>Параметры зарядки</h3><div class="field"><span class="label">Напряжение L1:</span><span class="value" id="voltagePhase1">-</span></div><div class="field"><span class="label">Напряжение L2:</span><span class="value" id="voltagePhase2">-</span></div><div class="field"><span class="label">Напряжение L3:</span><span class="value" id="voltagePhase3">-</span></div><div class="field"><span class="label">Ток L1:</span><span class="value" id="currentPhase1">-</span></div><div class="field"><span class="label">Ток L2:</span><span class="value" id="currentPhase2">-</span></div><div class="field"><span class="label">Ток L3:</span><span class="value" id="currentPhase3">-</span></div></div><div class="section"><h3>Статус зарядного устройства</h3><div class="field"><span class="label">Онлайн:</span><input type="checkbox" class="checkbox" id="chargerOnline" disabled></div><div class="field"><span class="label">Ошибка:</span><input type="checkbox" class="checkbox" id="chargerError" disabled></div><div class="field"><span class="label">Превышение мощности:</span><input type="checkbox" class="checkbox" id="powerOverconsumption" disabled></div><div class="field"><span class="label">Фиксированная мощность:</span><input type="checkbox" class="checkbox" id="fixedPower" disabled></div></div></div><script>function loadData(){fetch('/api/station').then(response=>response.json()).then(data=>{document.getElementById('station-name').textContent=data.displayName+' ('+data.technicalName+')';document.getElementById('displayName').textContent=data.displayName;document.getElementById('type').textContent=data.type;const statusEl=document.getElementById('status');statusEl.textContent=data.status;statusEl.className='status '+data.status;document.getElementById('maxPower').textContent=data.maxPower+' kW';document.getElementById('currentPower').textContent=data.currentPower+' kW';document.getElementById('ipAddress').textContent=data.ipAddress;document.getElementById('carConnection').checked=data.carConnection;document.getElementById('carChargingPermission').checked=data.carChargingPermission;document.getElementById('carError').checked=data.carError;document.getElementById('masterOnline').checked=data.masterOnline;document.getElementById('masterChargingPermission').checked=data.masterChargingPermission;document.getElementById('masterAvailablePower').textContent=data.masterAvailablePower+' kW';document.getElementById('voltagePhase1').textContent=data.voltagePhase1+' V';document.getElementById('voltagePhase2').textContent=data.voltagePhase2+' V';document.getElementById('voltagePhase3').textContent=data.voltagePhase3+' V';document.getElementById('currentPhase1').textContent=data.currentPhase1+' A';document.getElementById('currentPhase2').textContent=data.currentPhase2+' A';document.getElementById('currentPhase3').textContent=data.currentPhase3+' A';document.getElementById('chargerOnline').checked=data.chargerOnline;document.getElementById('chargerError').checked=data.chargerError;document.getElementById('powerOverconsumption').checked=data.powerOverconsumption;document.getElementById('fixedPower').checked=data.fixedPower}).catch(error=>{console.error('Ошибка загрузки данных:',error)})}setInterval(loadData,5000);loadData()</script></body></html>)rawliteral";

String getStationJSON() {
  StaticJsonDocument<1024> doc;
  
  doc["id"] = 1;
  doc["displayName"] = station.displayName;
  doc["technicalName"] = station.technicalName;
  doc["type"] = station.type;
  doc["maxPower"] = station.maxPower;
  doc["currentPower"] = station.currentPower;
  doc["status"] = station.status;
  doc["ipAddress"] = station.ipAddress;
  
  doc["carConnection"] = station.carConnection;
  doc["carChargingPermission"] = station.carChargingPermission;
  doc["carError"] = station.carError;
  
  doc["masterOnline"] = station.masterOnline;
  doc["masterChargingPermission"] = station.masterChargingPermission;
  doc["masterAvailablePower"] = station.masterAvailablePower;
  
  doc["voltagePhase1"] = station.voltagePhase1;
  doc["voltagePhase2"] = station.voltagePhase2;
  doc["voltagePhase3"] = station.voltagePhase3;
  doc["currentPhase1"] = station.currentPhase1;
  doc["currentPhase2"] = station.currentPhase2;
  doc["currentPhase3"] = station.currentPhase3;
  
  doc["chargerOnline"] = station.chargerOnline;
  doc["chargerError"] = station.chargerError;
  doc["powerOverconsumption"] = station.powerOverconsumption;
  doc["fixedPower"] = station.fixedPower;
  
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

void handleRoot() {
  server.send(200, "text/html", WEB_PAGE);
}

void handleAPI() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  if (server.method() == HTTP_OPTIONS) {
    server.send(200);
    return;
  }
  
  server.send(200, "application/json", getStationJSON());
}

void handleInfo() {
  StaticJsonDocument<512> doc;
  doc["id"] = "ESP32-001";
  doc["type"] = "slave";
  doc["ip"] = WiFi.localIP().toString();
  doc["name"] = station.displayName;
  doc["technicalName"] = station.technicalName;
  doc["maxPower"] = station.maxPower;
  doc["status"] = "online";
  doc["lastSeen"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== ESP32 Charging Station ===");
  
  // Подключение к WiFi
  WiFi.begin(ssid, password);
  Serial.print("Подключение к WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi подключен!");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());
  
  station.ipAddress = WiFi.localIP().toString();
  
  // Настройка mDNS
  if (MDNS.begin(hostname)) {
    Serial.println("mDNS запущен");
    MDNS.addService("http", "tcp", 80);
  }
  
  // Настройка веб-сервера
  server.on("/", handleRoot);
  server.on("/api/station", handleAPI);
  server.on("/api/info", handleInfo);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
  });
  
  server.begin();
  Serial.println("=========================");
  Serial.println("Веб-сервер запущен!");
  Serial.println("Доступ по адресам:");
  Serial.println("http://" + WiFi.localIP().toString());
  Serial.println("http://" + String(hostname) + ".local");
  Serial.println("=========================");
}

void loop() {
  server.handleClient();
  MDNS.update();
  
  // Симуляция изменения данных
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 10000) {
    // Генерируем случайные значения тока
    station.currentPhase1 = random(0, 32);
    station.currentPhase2 = random(0, 32);
    station.currentPhase3 = random(0, 32);
    
    // Рассчитываем общую мощность
    station.currentPower = (station.currentPhase1 + station.currentPhase2 + station.currentPhase3) * 0.23;
    
    // Симулируем подключение автомобиля
    if (random(0, 100) < 10) { // 10% шанс изменения
      station.carConnection = !station.carConnection;
    }
    
    lastUpdate = millis();
  }
  
  delay(10);
}