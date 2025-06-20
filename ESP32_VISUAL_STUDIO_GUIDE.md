# Развертывание веб-интерфейса на ESP32 с Visual Studio

Полное руководство по разработке и развертыванию системы зарядных станций на ESP32 с использованием Visual Studio Code и PlatformIO.

## Подготовка среды разработки

### 1. Установка Visual Studio Code

```bash
# Скачать и установить VS Code
https://code.visualstudio.com/

# Или через пакетный менеджер
# Windows (Chocolatey)
choco install vscode

# macOS (Homebrew)
brew install --cask visual-studio-code

# Ubuntu/Debian
wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg
sudo install -o root -g root -m 644 packages.microsoft.gpg /etc/apt/trusted.gpg.d/
sudo apt update
sudo apt install code
```

### 2. Установка PlatformIO Extension

1. Открыть VS Code
2. Перейти в Extensions (Ctrl+Shift+X)
3. Найти и установить "PlatformIO IDE"
4. Перезапустить VS Code

### 3. Установка драйверов ESP32

```bash
# Windows - CP210x USB to UART Bridge
https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

# macOS/Linux - обычно драйверы уже установлены
```

## Создание проекта

### 1. Создание нового проекта

1. Открыть PlatformIO Home (иконка дома в боковой панели)
2. Нажать "New Project"
3. Настроить проект:
   - **Name**: `charging_station_esp32`
   - **Board**: `Espressif ESP32 Dev Module`
   - **Framework**: `Arduino` или `Espressif IoT Development Framework`
   - **Location**: выбрать папку

### 2. Структура проекта

```
charging_station_esp32/
├── platformio.ini           # Конфигурация проекта
├── src/
│   └── main.cpp             # Основной файл (Arduino) или main.c (ESP-IDF)
├── include/
│   ├── web_interface.h      # HTML интерфейс
│   ├── station_config.h     # Конфигурация станции
│   └── api_handlers.h       # API обработчики
├── lib/                     # Локальные библиотеки
├── data/                    # Файлы для SPIFFS (веб-ресурсы)
└── test/                    # Тесты
```

## Конфигурация проекта

### 1. platformio.ini (Arduino Framework)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

# Библиотеки
lib_deps = 
    ESP Async WebServer
    ArduinoJson
    ESPAsyncTCP
    WiFi

# Настройки сборки
build_flags = 
    -DWIFI_SSID=\"YOUR_WIFI_NETWORK\"
    -DWIFI_PASSWORD=\"YOUR_WIFI_PASSWORD\"
    -DSTATION_ID=\"ESP32_STATION_001\"

# Настройки загрузки
monitor_speed = 115200
upload_speed = 921600

# SPIFFS для веб-файлов
board_build.filesystem = spiffs
```

### 2. platformio.ini (ESP-IDF Framework)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = espidf

# Компоненты ESP-IDF
lib_deps = 
    ESP-IDF

# Настройки сборки
build_flags = 
    -DCONFIG_HTTPD_MAX_REQ_HDR_LEN=8192
    -DCONFIG_HTTPD_MAX_URI_LEN=512

monitor_speed = 115200
upload_speed = 921600
```

## Реализация кода (Arduino Framework)

### 1. Основной файл (src/main.cpp)

```cpp
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "web_interface.h"
#include "station_config.h"

// Настройки WiFi
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Веб-сервер
AsyncWebServer server(80);

// Данные станции
struct StationData {
  String board_id = STATION_ID;
  String board_type = "slave"; // или "master"
  String display_name = "ESP32 Charging Station";
  String technical_name = "CS-ESP32-001";
  String status = "available";
  float max_power = 22.0;
  float current_power = 0.0;
  
  // Данные slave-станции
  bool car_connected = false;
  bool car_charging_permission = false;
  bool car_error = false;
  float voltage_phase1 = 230.0;
  float voltage_phase2 = 230.0;
  float voltage_phase3 = 230.0;
  float current_phase1 = 0.0;
  float current_phase2 = 0.0;
  float current_phase3 = 0.0;
  float charger_power = 0.0;
  bool single_phase_connection = false;
  bool power_overconsumption = false;
  bool fixed_power = false;
  
  // Связь с master
  bool master_online = true;
  bool master_charging_permission = true;
  float master_available_power = 100.0;
} stationData;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 Charging Station...");

  // Инициализация SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Подключение к WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Настройка веб-сервера
  setupWebServer();
  
  // Запуск сервера
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Open http://" + WiFi.localIP().toString() + "/ in browser");
}

void loop() {
  // Обновление данных станции
  updateStationData();
  delay(1000);
}

void setupWebServer() {
  // CORS заголовки
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  // Главная страница
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", web_interface_html);
  });

  // API: Информация о плате
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    doc["device_type"] = "charging_station";
    doc["board_type"] = stationData.board_type;
    doc["board_id"] = stationData.board_id;
    doc["display_name"] = stationData.display_name;
    doc["technical_name"] = stationData.technical_name;
    doc["max_power"] = stationData.max_power;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API: Подключение
  server.on("/api/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);
    doc["status"] = "connected";
    doc["message"] = "Подключение успешно";
    doc["timestamp"] = millis();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API: Получение данных
  server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(2048);
    
    doc["status"] = stationData.status;
    doc["current_power"] = stationData.current_power;

    // Данные slave-станции
    if (stationData.board_type == "slave") {
      JsonObject slave_data = doc.createNestedObject("slave_data");
      slave_data["car_connected"] = stationData.car_connected;
      slave_data["car_charging_permission"] = stationData.car_charging_permission;
      slave_data["car_error"] = stationData.car_error;
      slave_data["voltage_phase1"] = stationData.voltage_phase1;
      slave_data["voltage_phase2"] = stationData.voltage_phase2;
      slave_data["voltage_phase3"] = stationData.voltage_phase3;
      slave_data["current_phase1"] = stationData.current_phase1;
      slave_data["current_phase2"] = stationData.current_phase2;
      slave_data["current_phase3"] = stationData.current_phase3;
      slave_data["charger_power"] = stationData.charger_power;
      slave_data["single_phase_connection"] = stationData.single_phase_connection;
      slave_data["power_overconsumption"] = stationData.power_overconsumption;
      slave_data["fixed_power"] = stationData.fixed_power;

      JsonObject master_connection = doc.createNestedObject("master_connection");
      master_connection["master_online"] = stationData.master_online;
      master_connection["master_charging_permission"] = stationData.master_charging_permission;
      master_connection["master_available_power"] = stationData.master_available_power;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API: Команды управления
  server.on("/api/data", HTTP_POST, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    DynamicJsonDocument requestDoc(1024);
    deserializeJson(requestDoc, (char*)data);

    String command = requestDoc["command"];
    DynamicJsonDocument responseDoc(512);

    if (command == "update_settings") {
      JsonObject settings = requestDoc["settings"];
      if (settings.containsKey("charging_permission")) {
        stationData.car_charging_permission = settings["charging_permission"];
      }
      if (settings.containsKey("max_power")) {
        stationData.max_power = settings["max_power"];
      }
      if (settings.containsKey("fixed_power")) {
        stationData.fixed_power = settings["fixed_power"];
      }

      responseDoc["status"] = "success";
      responseDoc["message"] = "Настройки обновлены";
      
      JsonObject applied = responseDoc.createNestedObject("applied_settings");
      applied["charging_permission"] = stationData.car_charging_permission;
      applied["max_power"] = stationData.max_power;
      applied["fixed_power"] = stationData.fixed_power;
    } 
    else if (command == "toggle_charging") {
      stationData.car_charging_permission = !stationData.car_charging_permission;
      stationData.status = stationData.car_charging_permission ? "charging" : "available";
      
      responseDoc["status"] = "success";
      responseDoc["message"] = "Режим зарядки изменен";
      responseDoc["charging"] = stationData.car_charging_permission;
    }
    else if (command == "emergency_stop") {
      stationData.car_charging_permission = false;
      stationData.status = "offline";
      stationData.current_power = 0.0;
      
      responseDoc["status"] = "success";
      responseDoc["message"] = "Аварийная остановка выполнена";
    }
    else {
      responseDoc["status"] = "error";
      responseDoc["message"] = "Неизвестная команда";
    }

    String response;
    serializeJson(responseDoc, response);
    request->send(200, "application/json", response);
  });

  // OPTIONS для CORS
  server.on("/api/info", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    request->send(200);
  });
  server.on("/api/connect", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    request->send(200);
  });
  server.on("/api/data", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    request->send(200);
  });
}

void updateStationData() {
  // Симуляция изменения данных
  static unsigned long lastUpdate = 0;
  static bool simulation_direction = true;
  
  if (millis() - lastUpdate > 2000) { // Каждые 2 секунды
    lastUpdate = millis();
    
    if (stationData.car_charging_permission && stationData.car_connected) {
      // Симуляция зарядки
      if (simulation_direction) {
        stationData.current_power += 0.5;
        stationData.current_phase1 += 0.2;
        stationData.current_phase2 += 0.2;
        stationData.current_phase3 += 0.2;
        if (stationData.current_power >= stationData.max_power) {
          simulation_direction = false;
        }
      } else {
        stationData.current_power -= 0.5;
        stationData.current_phase1 -= 0.2;
        stationData.current_phase2 -= 0.2;
        stationData.current_phase3 -= 0.2;
        if (stationData.current_power <= 0) {
          simulation_direction = true;
        }
      }
      
      stationData.charger_power = stationData.current_power;
      stationData.status = "charging";
    } else {
      stationData.current_power = 0.0;
      stationData.current_phase1 = 0.0;
      stationData.current_phase2 = 0.0;
      stationData.current_phase3 = 0.0;
      stationData.charger_power = 0.0;
      stationData.status = "available";
    }

    // Случайное подключение/отключение автомобиля
    if (random(100) < 5) { // 5% шанс изменения
      stationData.car_connected = !stationData.car_connected;
    }

    // Небольшие колебания напряжения
    stationData.voltage_phase1 = 230.0 + random(-10, 11);
    stationData.voltage_phase2 = 230.0 + random(-10, 11);
    stationData.voltage_phase3 = 230.0 + random(-10, 11);
  }
}
```

### 2. Заголовочный файл веб-интерфейса (include/web_interface.h)

```cpp
#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

const char web_interface_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Charging Station</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container { 
            max-width: 1200px; 
            margin: 0 auto; 
            background: white; 
            border-radius: 15px; 
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #4CAF50 0%, #45a049 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 { font-size: 2.5em; margin-bottom: 10px; }
        .header p { font-size: 1.2em; opacity: 0.9; }
        .content { padding: 30px; }
        .grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); 
            gap: 25px; 
        }
        .card { 
            background: #f8f9fa; 
            border-radius: 12px; 
            padding: 25px; 
            box-shadow: 0 5px 15px rgba(0,0,0,0.08);
            transition: transform 0.3s ease;
        }
        .card:hover { transform: translateY(-5px); }
        .card h3 { 
            color: #333; 
            margin-bottom: 20px; 
            font-size: 1.4em;
            border-bottom: 2px solid #4CAF50;
            padding-bottom: 10px;
        }
        .status { 
            display: inline-block;
            padding: 8px 16px; 
            border-radius: 25px; 
            color: white; 
            font-weight: bold; 
            font-size: 0.9em;
            margin-bottom: 15px;
        }
        .charging { background: linear-gradient(135deg, #4CAF50, #45a049); }
        .available { background: linear-gradient(135deg, #2196F3, #1976D2); }
        .offline { background: linear-gradient(135deg, #f44336, #d32f2f); }
        .maintenance { background: linear-gradient(135deg, #ff9800, #f57c00); }
        
        .metric { 
            display: flex; 
            justify-content: space-between; 
            align-items: center;
            padding: 12px 0; 
            border-bottom: 1px solid #e0e0e0;
        }
        .metric:last-child { border-bottom: none; }
        .metric-label { color: #666; font-weight: 500; }
        .metric-value { 
            font-weight: bold; 
            color: #333; 
            font-size: 1.1em;
        }
        
        .controls {
            display: flex;
            flex-wrap: wrap;
            gap: 15px;
            margin-top: 20px;
        }
        button { 
            flex: 1;
            min-width: 140px;
            padding: 12px 20px; 
            border: none; 
            border-radius: 8px; 
            cursor: pointer; 
            font-size: 1em;
            font-weight: 600;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .btn-primary { 
            background: linear-gradient(135deg, #2196F3, #1976D2); 
            color: white; 
        }
        .btn-primary:hover { 
            background: linear-gradient(135deg, #1976D2, #1565C0); 
            transform: translateY(-2px);
        }
        .btn-danger { 
            background: linear-gradient(135deg, #f44336, #d32f2f); 
            color: white; 
        }
        .btn-danger:hover { 
            background: linear-gradient(135deg, #d32f2f, #c62828); 
            transform: translateY(-2px);
        }
        .btn-success { 
            background: linear-gradient(135deg, #4CAF50, #45a049); 
            color: white; 
        }
        .btn-success:hover { 
            background: linear-gradient(135deg, #45a049, #388e3c); 
            transform: translateY(-2px);
        }
        
        .loading { 
            text-align: center; 
            color: #666; 
            font-style: italic;
        }
        
        @media (max-width: 768px) {
            .grid { grid-template-columns: 1fr; }
            .controls { flex-direction: column; }
            button { min-width: 100%; }
        }
        
        .connection-status {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 10px 15px;
            border-radius: 20px;
            color: white;
            font-weight: bold;
            z-index: 1000;
        }
        .connected { background: #4CAF50; }
        .disconnected { background: #f44336; }
    </style>
</head>
<body>
    <div class="connection-status" id="connectionStatus">Connected</div>
    
    <div class="container">
        <div class="header">
            <h1>⚡ ESP32 Charging Station</h1>
            <p>Advanced Electric Vehicle Charging Management System</p>
        </div>
        
        <div class="content">
            <div class="grid">
                <!-- Station Status Card -->
                <div class="card">
                    <h3>🔋 Station Status</h3>
                    <div id="stationStatus" class="loading">Loading...</div>
                    <div class="metric">
                        <span class="metric-label">Station ID:</span>
                        <span class="metric-value" id="stationId">-</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Current Power:</span>
                        <span class="metric-value" id="currentPower">- kW</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Max Power:</span>
                        <span class="metric-value" id="maxPower">- kW</span>
                    </div>
                </div>

                <!-- Power Metrics Card -->
                <div class="card">
                    <h3>⚡ Power Metrics</h3>
                    <div class="metric">
                        <span class="metric-label">Phase 1 Voltage:</span>
                        <span class="metric-value" id="voltage1">- V</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Phase 2 Voltage:</span>
                        <span class="metric-value" id="voltage2">- V</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Phase 3 Voltage:</span>
                        <span class="metric-value" id="voltage3">- V</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Charger Power:</span>
                        <span class="metric-value" id="chargerPower">- kW</span>
                    </div>
                </div>

                <!-- Vehicle Status Card -->
                <div class="card">
                    <h3>🚗 Vehicle Status</h3>
                    <div class="metric">
                        <span class="metric-label">Vehicle Connected:</span>
                        <span class="metric-value" id="carConnected">-</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Charging Permission:</span>
                        <span class="metric-value" id="chargingPermission">-</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Vehicle Error:</span>
                        <span class="metric-value" id="carError">-</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Master Online:</span>
                        <span class="metric-value" id="masterOnline">-</span>
                    </div>
                </div>

                <!-- Control Panel Card -->
                <div class="card">
                    <h3>🎛️ Control Panel</h3>
                    <div class="controls">
                        <button class="btn-primary" onclick="toggleCharging()">Toggle Charging</button>
                        <button class="btn-danger" onclick="emergencyStop()">Emergency Stop</button>
                        <button class="btn-success" onclick="refreshData()">Refresh Data</button>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script>
        let stationData = {};
        let isConnected = true;

        async function fetchStationData() {
            try {
                const response = await fetch('/api/data');
                if (!response.ok) throw new Error('Network response was not ok');
                
                stationData = await response.json();
                updateInterface();
                updateConnectionStatus(true);
            } catch (error) {
                console.error('Error fetching data:', error);
                updateConnectionStatus(false);
            }
        }

        function updateInterface() {
            // Station Status
            const statusElement = document.getElementById('stationStatus');
            statusElement.innerHTML = `<span class="status ${stationData.status}">${stationData.status.toUpperCase()}</span>`;
            
            document.getElementById('currentPower').textContent = `${stationData.current_power || 0} kW`;
            
            // Power Metrics
            if (stationData.slave_data) {
                const data = stationData.slave_data;
                document.getElementById('voltage1').textContent = `${data.voltage_phase1 || 0} V`;
                document.getElementById('voltage2').textContent = `${data.voltage_phase2 || 0} V`;
                document.getElementById('voltage3').textContent = `${data.voltage_phase3 || 0} V`;
                document.getElementById('chargerPower').textContent = `${data.charger_power || 0} kW`;
                
                // Vehicle Status
                document.getElementById('carConnected').textContent = data.car_connected ? 'Yes' : 'No';
                document.getElementById('chargingPermission').textContent = data.car_charging_permission ? 'Granted' : 'Denied';
                document.getElementById('carError').textContent = data.car_error ? 'Error' : 'OK';
                
                if (stationData.master_connection) {
                    document.getElementById('masterOnline').textContent = 
                        stationData.master_connection.master_online ? 'Online' : 'Offline';
                }
            }
        }

        function updateConnectionStatus(connected) {
            const statusElement = document.getElementById('connectionStatus');
            if (connected !== isConnected) {
                isConnected = connected;
                statusElement.textContent = connected ? 'Connected' : 'Disconnected';
                statusElement.className = `connection-status ${connected ? 'connected' : 'disconnected'}`;
            }
        }

        async function sendCommand(command, data = {}) {
            try {
                const response = await fetch('/api/data', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ command, ...data })
                });
                
                if (!response.ok) throw new Error('Command failed');
                
                const result = await response.json();
                console.log('Command result:', result);
                
                // Обновляем данные после команды
                setTimeout(fetchStationData, 500);
                
                return result;
            } catch (error) {
                console.error('Error sending command:', error);
                alert('Error: ' + error.message);
            }
        }

        async function toggleCharging() {
            await sendCommand('toggle_charging');
        }

        async function emergencyStop() {
            if (confirm('Are you sure you want to perform emergency stop?')) {
                await sendCommand('emergency_stop');
            }
        }

        function refreshData() {
            fetchStationData();
        }

        // Инициализация и автообновление
        document.addEventListener('DOMContentLoaded', () => {
            fetchStationData();
            
            // Загружаем информацию о станции
            fetch('/api/info')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('stationId').textContent = data.board_id || 'Unknown';
                    document.getElementById('maxPower').textContent = `${data.max_power || 0} kW`;
                })
                .catch(error => console.error('Error fetching station info:', error));
        });

        // Автообновление каждые 3 секунды
        setInterval(fetchStationData, 3000);
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_INTERFACE_H
```

### 3. Конфигурационный файл (include/station_config.h)

```cpp
#ifndef STATION_CONFIG_H
#define STATION_CONFIG_H

// WiFi настройки (можно переопределить в platformio.ini)
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_NETWORK"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

// Настройки станции
#ifndef STATION_ID
#define STATION_ID "ESP32_STATION_001"
#endif

// Настройки сервера
#define HTTP_PORT 80
#define UPDATE_INTERVAL 1000  // мс

// Настройки симуляции
#define SIMULATION_ENABLED true
#define MAX_SIMULATION_POWER 22.0
#define MIN_SIMULATION_POWER 0.0

// Пины для реальных датчиков (если нужно)
#define VOLTAGE_SENSOR_PIN_1 34
#define VOLTAGE_SENSOR_PIN_2 35
#define VOLTAGE_SENSOR_PIN_3 32
#define CURRENT_SENSOR_PIN_1 33
#define CURRENT_SENSOR_PIN_2 25
#define CURRENT_SENSOR_PIN_3 26

// Реле управления
#define RELAY_PIN_1 16
#define RELAY_PIN_2 17
#define RELAY_PIN_3 18

// Индикаторы
#define LED_STATUS_PIN 2
#define LED_CHARGING_PIN 4
#define LED_ERROR_PIN 5

#endif // STATION_CONFIG_H
```

## Сборка и загрузка

### 1. Сборка проекта

1. Открыть проект в VS Code
2. В PlatformIO Toolbar нажать "Build" (галочка)
3. Дождаться завершения сборки

### 2. Загрузка на ESP32

1. Подключить ESP32 к компьютеру через USB
2. В PlatformIO Toolbar нажать "Upload" (стрелка)
3. Дождаться завершения загрузки

### 3. Мониторинг

1. В PlatformIO Toolbar нажать "Serial Monitor" (вилка)
2. Следить за логами и IP адресом ESP32

## Отладка в VS Code

### 1. Настройка отладки

Создать файл `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "platformio-debug",
            "request": "launch",
            "name": "PIO Debug",
            "executable": ".pio/build/esp32dev/firmware.elf",
            "projectDir": "${workspaceFolder}",
            "toolchainBinDir": "${env:HOME}/.platformio/packages/toolchain-xtensa32/bin",
            "svdPath": "${env:HOME}/.platformio/platforms/espressif32/misc/svd/esp32.svd"
        }
    ]
}
```

### 2. Использование Serial Plotter

1. View → Command Palette → "PlatformIO: Serial Plotter"
2. Выводить данные в формате: `voltage1:230,current1:15.5,power:11.2`

## Развертывание в сети

### 1. Статический IP (опционально)

Добавить в `setup()`:

```cpp
// Статический IP
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

if (!WiFi.config(local_IP, gateway, subnet)) {
  Serial.println("STA Failed to configure");
}
```

### 2. mDNS для удобного доступа

Добавить библиотеку в `platformio.ini`:

```ini
lib_deps = 
    ESPmDNS
```

В коде:

```cpp
#include <ESPmDNS.h>

// В setup() после подключения WiFi
if (MDNS.begin("charging-station")) {
  Serial.println("MDNS responder started");
  Serial.println("Access: http://charging-station.local/");
}
```

## Обновление прошивки OTA

### 1. Добавить библиотеку

```ini
lib_deps = 
    ArduinoOTA
```

### 2. Настроить OTA

```cpp
#include <ArduinoOTA.h>

void setupOTA() {
  ArduinoOTA.setHostname("charging-station");
  ArduinoOTA.setPassword("your_ota_password");
  
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  });
  
  ArduinoOTA.begin();
}

// В loop()
ArduinoOTA.handle();
```

### 3. Обновление через VS Code

1. В PlatformIO: Upload → Upload using OTA
2. Выбрать сетевое устройство
3. Ввести пароль OTA

Это руководство обеспечивает полную разработку и развертывание веб-интерфейса ESP32 с использованием современных инструментов Visual Studio Code и PlatformIO.