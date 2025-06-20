# Развертывание веб-интерфейса на ESP32 плате

Полное руководство по установке и настройке веб-интерфейса системы зарядных станций на ESP32 микроконтроллере.

## Подготовка среды разработки

### 1. Установка ESP-IDF

```bash
# Клонирование ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Установка для ESP32
./install.sh esp32

# Настройка переменных окружения
source ./export.sh
```

### 2. Проверка установки

```bash
idf.py --version
# Должно показать: ESP-IDF v5.0+
```

## Структура проекта для ESP32

### 1. Создание проекта

```bash
mkdir charging_station_esp32
cd charging_station_esp32
idf.py create-project charging_station
cd charging_station
```

### 2. Структура файлов

```
charging_station/
├── main/
│   ├── main.c                    # Точка входа
│   ├── http_server.c             # HTTP сервер
│   ├── wifi_manager.c            # WiFi подключение
│   ├── station_data.c            # Управление данными станции
│   ├── web_interface.h           # Встроенный HTML
│   ├── api_handlers.c            # API обработчики
│   └── CMakeLists.txt           # Конфигурация сборки
├── components/                   # Дополнительные компоненты
├── partitions.csv               # Таблица разделов памяти
├── sdkconfig.defaults           # Настройки по умолчанию
└── CMakeLists.txt              # Основная конфигурация
```

## Конфигурация WiFi

### 1. Настройка WiFi менеджера (wifi_manager.c)

```c
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

// Настройки WiFi (замените на ваши)
#define WIFI_SSID      "YOUR_WIFI_NETWORK"
#define WIFI_PASS      "YOUR_WIFI_PASSWORD"
#define MAX_RETRY      5

static const char *TAG = "WiFi";
static int retry_count = 0;

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                       int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < MAX_RETRY) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "Retry WiFi connection (%d/%d)", retry_count, MAX_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_count = 0;
    }
}

esp_err_t wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    return ESP_OK;
}
```

### 2. Встроенный веб-интерфейс (web_interface.h)

```c
#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

// Минифицированный HTML интерфейс
static const char web_interface_html[] = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Charging Station Control</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }
        .card { border: 1px solid #ddd; padding: 15px; margin: 10px 0; border-radius: 5px; }
        .status { padding: 5px 10px; border-radius: 3px; color: white; font-weight: bold; }
        .charging { background: #4CAF50; }
        .available { background: #2196F3; }
        .offline { background: #f44336; }
        .maintenance { background: #ff9800; }
        button { padding: 10px 20px; margin: 5px; border: none; border-radius: 3px; cursor: pointer; }
        .primary { background: #2196F3; color: white; }
        .danger { background: #f44336; color: white; }
        input, select { padding: 8px; margin: 5px; border: 1px solid #ddd; border-radius: 3px; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Charging Station Control Panel</h1>
        
        <div class="card">
            <h3>Station Status</h3>
            <div id="stationInfo">Loading...</div>
        </div>

        <div class="card">
            <h3>Power Management</h3>
            <div class="grid">
                <div>
                    <label>Phase 1 Voltage:</label>
                    <span id="voltage1">-</span> V
                </div>
                <div>
                    <label>Phase 2 Voltage:</label>
                    <span id="voltage2">-</span> V
                </div>
                <div>
                    <label>Phase 3 Voltage:</label>
                    <span id="voltage3">-</span> V
                </div>
                <div>
                    <label>Current Power:</label>
                    <span id="currentPower">-</span> kW
                </div>
            </div>
        </div>

        <div class="card">
            <h3>Controls</h3>
            <button class="primary" onclick="toggleCharging()">Toggle Charging</button>
            <button class="danger" onclick="emergencyStop()">Emergency Stop</button>
            <button class="primary" onclick="refreshData()">Refresh</button>
        </div>
    </div>

    <script>
        let stationData = {};

        async function fetchStationData() {
            try {
                const response = await fetch('/api/data');
                stationData = await response.json();
                updateInterface();
            } catch (error) {
                console.error('Error fetching data:', error);
            }
        }

        function updateInterface() {
            document.getElementById('stationInfo').innerHTML = 
                `<span class="status ${stationData.status}">${stationData.status.toUpperCase()}</span>`;
            
            if (stationData.slave_data) {
                document.getElementById('voltage1').textContent = stationData.slave_data.voltage_phase1 || 0;
                document.getElementById('voltage2').textContent = stationData.slave_data.voltage_phase2 || 0;
                document.getElementById('voltage3').textContent = stationData.slave_data.voltage_phase3 || 0;
                document.getElementById('currentPower').textContent = stationData.current_power || 0;
            }
        }

        async function toggleCharging() {
            try {
                const response = await fetch('/api/data', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        command: 'toggle_charging'
                    })
                });
                const result = await response.json();
                console.log('Toggle result:', result);
                fetchStationData();
            } catch (error) {
                console.error('Error toggling charging:', error);
            }
        }

        async function emergencyStop() {
            if (confirm('Are you sure you want to perform emergency stop?')) {
                try {
                    await fetch('/api/data', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({
                            command: 'emergency_stop'
                        })
                    });
                    fetchStationData();
                } catch (error) {
                    console.error('Error emergency stop:', error);
                }
            }
        }

        function refreshData() {
            fetchStationData();
        }

        // Автообновление каждые 5 секунд
        setInterval(fetchStationData, 5000);
        fetchStationData();
    </script>
</body>
</html>
)html";

#endif // WEB_INTERFACE_H
```

## HTTP Server Implementation

### 1. Основной HTTP сервер (http_server.c)

```c
#include "esp_http_server.h"
#include "cJSON.h"
#include "web_interface.h"

static const char *TAG = "HTTP_SERVER";

// Обработчик главной страницы
static esp_err_t web_interface_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, web_interface_html, strlen(web_interface_html));
    return ESP_OK;
}

// Обработчик /api/info
static esp_err_t api_info_handler(httpd_req_t *req)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "device_type", "charging_station");
    cJSON_AddStringToObject(json, "board_type", "slave"); // или "master"
    cJSON_AddStringToObject(json, "board_id", "ESP32_STATION_001");
    cJSON_AddStringToObject(json, "display_name", "ESP32 Charging Station");
    cJSON_AddStringToObject(json, "technical_name", "CS-ESP32-001");
    cJSON_AddNumberToObject(json, "max_power", 22.0);

    char *json_string = cJSON_Print(json);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(json);
    return ESP_OK;
}

// Запуск HTTP сервера
httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;
    config.stack_size = 8192;

    httpd_handle_t server = NULL;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Регистрация обработчиков
        httpd_uri_t web_interface_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = web_interface_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &web_interface_uri);

        httpd_uri_t api_info_uri = {
            .uri = "/api/info",
            .method = HTTP_GET,
            .handler = api_info_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_info_uri);

        ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    }
    
    return server;
}
```

## Основной файл приложения (main.c)

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

extern esp_err_t wifi_init_sta(void);
extern httpd_handle_t start_webserver(void);

static const char *TAG = "CHARGING_STATION";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Charging Station ESP32");

    // Инициализация NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Инициализация WiFi
    wifi_init_sta();
    
    // Задержка для подключения к WiFi
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
    // Запуск веб-сервера
    start_webserver();
    
    ESP_LOGI(TAG, "Charging Station ready!");
    ESP_LOGI(TAG, "Open http://[ESP32_IP]/ in your browser");
}
```

## Конфигурация сборки

### 1. CMakeLists.txt (корневой)

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(charging_station)
```

### 2. main/CMakeLists.txt

```cmake
idf_component_register(SRCS "main.c"
                           "http_server.c"
                           "wifi_manager.c"
                           "station_data.c"
                           "api_handlers.c"
                      INCLUDE_DIRS ".")
```

### 3. partitions.csv

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 1M,
```

### 4. sdkconfig.defaults

```ini
# WiFi Configuration
CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32

# HTTP Server
CONFIG_HTTPD_MAX_REQ_HDR_LEN=8192
CONFIG_HTTPD_MAX_URI_LEN=512

# Memory
CONFIG_MAIN_TASK_STACK_SIZE=8192
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10

# Logging
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
```

## Сборка и прошивка

### 1. Настройка проекта

```bash
cd charging_station
idf.py menuconfig
```

Важные настройки:
- Component config → Wi-Fi → установите SSID и пароль
- Component config → HTTP Server → увеличьте буферы при необходимости

### 2. Сборка

```bash
idf.py build
```

### 3. Прошивка

```bash
# Подключите ESP32 через USB
idf.py -p /dev/ttyUSB0 flash monitor
```

## Тестирование

### 1. Поиск IP адреса ESP32

```bash
# Сканирование сети
nmap -sn 192.168.1.0/24 | grep -A 2 "ESP32"

# Или проверка в логах ESP32
idf.py monitor
```

### 2. Проверка веб-интерфейса

```bash
# Открыть в браузере
http://192.168.1.XXX/

# Проверить API
curl http://192.168.1.XXX/api/info
```

## Отладка и мониторинг

### 1. Просмотр логов

```bash
idf.py monitor
```

### 2. Удаленный мониторинг

```bash
# Через telnet (если настроен)
telnet 192.168.1.XXX 23

# Через HTTP API
curl http://192.168.1.XXX/api/data
```

## Обновление прошивки

### 1. OTA обновления (опционально)

Добавьте поддержку обновлений по воздуху для удаленного обновления прошивки.

### 2. Ручное обновление

```bash
idf.py -p /dev/ttyUSB0 flash
```

Этот гид обеспечивает полное развертывание веб-интерфейса на ESP32 с поддержкой WiFi, HTTP сервера и API для интеграции с центральной системой управления зарядными станциями.