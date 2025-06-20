
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "web_interface_react.h"

static const char *TAG = "REACT_SERVER";

// Структура данных slave станции (аналогично React компоненту)
typedef struct {
    bool car_connection;
    bool car_charging_permission;
    bool car_error;
    bool master_online;
    bool master_charging_permission;
    float master_available_power;
    float voltage_phase1;
    float voltage_phase2;
    float voltage_phase3;
    float current_phase1;
    float current_phase2;
    float current_phase3;
    float charger_power;
    bool single_phase_connection;
    bool power_overconsumption;
    bool fixed_power;
} slave_data_t;

// Глобальные данные станции
static slave_data_t g_slave_data = {
    .car_connection = false,
    .car_charging_permission = false,
    .car_error = false,
    .master_online = true,
    .master_charging_permission = true,
    .master_available_power = 50.0,
    .voltage_phase1 = 220.0,
    .voltage_phase2 = 220.0,
    .voltage_phase3 = 220.0,
    .current_phase1 = 0.0,
    .current_phase2 = 0.0,
    .current_phase3 = 0.0,
    .charger_power = 0.0,
    .single_phase_connection = false,
    .power_overconsumption = false,
    .fixed_power = false
};

static char g_station_status[32] = "available";

// Обработчик главной страницы React интерфейса
static esp_err_t react_interface_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving React interface");
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
    
    esp_err_t result = httpd_resp_send(req, web_interface_react_html, HTTPD_RESP_USE_STRLEN);
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "React interface served successfully");
    } else {
        ESP_LOGE(TAG, "Failed to serve React interface: %s", esp_err_to_name(result));
    }
    
    return result;
}

// API обработчик информации о станции (аналогично React API)
static esp_err_t api_info_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "API info request");
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "device_type", "charging_station");
    cJSON_AddStringToObject(json, "board_type", "slave");
    cJSON_AddStringToObject(json, "board_id", "ESP32_SLAVE_001");
    cJSON_AddStringToObject(json, "display_name", "ESP32 Charging Station");
    cJSON_AddStringToObject(json, "technical_name", "esp32-slave-001");
    cJSON_AddNumberToObject(json, "max_power", 22.0);
    cJSON_AddStringToObject(json, "version", "1.0.0");
    cJSON_AddStringToObject(json, "interface_type", "react");

    char *json_string = cJSON_Print(json);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(json);
    return ESP_OK;
}

// API обработчик данных (GET/POST как в React приложении)
static esp_err_t api_data_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        // Возвращаем текущие данные (аналогично React useQuery)
        ESP_LOGI(TAG, "API data GET request");
        
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "status", g_station_status);
        cJSON_AddNumberToObject(json, "current_power", g_slave_data.charger_power);
        
        cJSON *slave_data = cJSON_CreateObject();
        cJSON_AddBoolToObject(slave_data, "car_connected", g_slave_data.car_connection);
        cJSON_AddBoolToObject(slave_data, "car_charging_permission", g_slave_data.car_charging_permission);
        cJSON_AddBoolToObject(slave_data, "car_error", g_slave_data.car_error);
        cJSON_AddBoolToObject(slave_data, "master_online", g_slave_data.master_online);
        cJSON_AddBoolToObject(slave_data, "master_charging_permission", g_slave_data.master_charging_permission);
        cJSON_AddNumberToObject(slave_data, "master_available_power", g_slave_data.master_available_power);
        cJSON_AddNumberToObject(slave_data, "voltage_phase1", g_slave_data.voltage_phase1);
        cJSON_AddNumberToObject(slave_data, "voltage_phase2", g_slave_data.voltage_phase2);
        cJSON_AddNumberToObject(slave_data, "voltage_phase3", g_slave_data.voltage_phase3);
        cJSON_AddNumberToObject(slave_data, "current_phase1", g_slave_data.current_phase1);
        cJSON_AddNumberToObject(slave_data, "current_phase2", g_slave_data.current_phase2);
        cJSON_AddNumberToObject(slave_data, "current_phase3", g_slave_data.current_phase3);
        cJSON_AddNumberToObject(slave_data, "charger_power", g_slave_data.charger_power);
        cJSON_AddBoolToObject(slave_data, "single_phase_connection", g_slave_data.single_phase_connection);
        cJSON_AddBoolToObject(slave_data, "power_overconsumption", g_slave_data.power_overconsumption);
        cJSON_AddBoolToObject(slave_data, "fixed_power", g_slave_data.fixed_power);
        
        cJSON_AddItemToObject(json, "slave_data", slave_data);
        
        char *json_string = cJSON_Print(json);
        
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json_string, strlen(json_string));
        
        free(json_string);
        cJSON_Delete(json);
        
    } else if (req->method == HTTP_POST) {
        // Обновляем данные (аналогично React useMutation)
        ESP_LOGI(TAG, "API data POST request");
        
        char content[1024];
        size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
        
        int ret = httpd_req_recv(req, content, recv_size);
        if (ret <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive data");
            return ESP_FAIL;
        }
        content[ret] = '\0';
        
        ESP_LOGI(TAG, "Received data: %s", content);
        
        cJSON *json = cJSON_Parse(content);
        if (json == NULL) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }
        
        // Обработка команд (как в React)
        cJSON *command = cJSON_GetObjectItem(json, "command");
        if (command != NULL && cJSON_IsString(command)) {
            const char *cmd = command->valuestring;
            
            if (strcmp(cmd, "update_slave_data") == 0) {
                // Обновление данных slave (аналогично React handleCheckboxChange/handleNumberChange)
                cJSON *slave_data = cJSON_GetObjectItem(json, "slave_data");
                if (slave_data != NULL) {
                    cJSON *item;
                    
                    // Булевые поля
                    item = cJSON_GetObjectItem(slave_data, "carConnection");
                    if (item != NULL && cJSON_IsBool(item)) {
                        g_slave_data.car_connection = cJSON_IsTrue(item);
                        ESP_LOGI(TAG, "Updated car_connection: %d", g_slave_data.car_connection);
                    }
                    
                    item = cJSON_GetObjectItem(slave_data, "carChargingPermission");
                    if (item != NULL && cJSON_IsBool(item)) {
                        g_slave_data.car_charging_permission = cJSON_IsTrue(item);
                        ESP_LOGI(TAG, "Updated car_charging_permission: %d", g_slave_data.car_charging_permission);
                    }
                    
                    item = cJSON_GetObjectItem(slave_data, "carError");
                    if (item != NULL && cJSON_IsBool(item)) {
                        g_slave_data.car_error = cJSON_IsTrue(item);
                        ESP_LOGI(TAG, "Updated car_error: %d", g_slave_data.car_error);
                    }
                    
                    // Числовые поля (напряжения)
                    item = cJSON_GetObjectItem(slave_data, "voltagePhase1");
                    if (item != NULL && cJSON_IsNumber(item)) {
                        g_slave_data.voltage_phase1 = (float)cJSON_GetNumberValue(item);
                        ESP_LOGI(TAG, "Updated voltage_phase1: %.1f", g_slave_data.voltage_phase1);
                    }
                    
                    item = cJSON_GetObjectItem(slave_data, "voltagePhase2");
                    if (item != NULL && cJSON_IsNumber(item)) {
                        g_slave_data.voltage_phase2 = (float)cJSON_GetNumberValue(item);
                    }
                    
                    item = cJSON_GetObjectItem(slave_data, "voltagePhase3");
                    if (item != NULL && cJSON_IsNumber(item)) {
                        g_slave_data.voltage_phase3 = (float)cJSON_GetNumberValue(item);
                    }
                    
                    // Токи
                    item = cJSON_GetObjectItem(slave_data, "currentPhase1");
                    if (item != NULL && cJSON_IsNumber(item)) {
                        g_slave_data.current_phase1 = (float)cJSON_GetNumberValue(item);
                    }
                    
                    item = cJSON_GetObjectItem(slave_data, "currentPhase2");
                    if (item != NULL && cJSON_IsNumber(item)) {
                        g_slave_data.current_phase2 = (float)cJSON_GetNumberValue(item);
                    }
                    
                    item = cJSON_GetObjectItem(slave_data, "currentPhase3");
                    if (item != NULL && cJSON_IsNumber(item)) {
                        g_slave_data.current_phase3 = (float)cJSON_GetNumberValue(item);
                    }
                    
                    // Мощность
                    item = cJSON_GetObjectItem(slave_data, "chargerPower");
                    if (item != NULL && cJSON_IsNumber(item)) {
                        g_slave_data.charger_power = (float)cJSON_GetNumberValue(item);
                    }
                    
                    // Статусные флаги
                    item = cJSON_GetObjectItem(slave_data, "singlePhaseConnection");
                    if (item != NULL && cJSON_IsBool(item)) {
                        g_slave_data.single_phase_connection = cJSON_IsTrue(item);
                    }
                    
                    item = cJSON_GetObjectItem(slave_data, "powerOverconsumption");
                    if (item != NULL && cJSON_IsBool(item)) {
                        g_slave_data.power_overconsumption = cJSON_IsTrue(item);
                    }
                    
                    item = cJSON_GetObjectItem(slave_data, "fixedPower");
                    if (item != NULL && cJSON_IsBool(item)) {
                        g_slave_data.fixed_power = cJSON_IsTrue(item);
                    }
                }
                
                strcpy(g_station_status, "updated");
                
            } else if (strcmp(cmd, "toggle_charging") == 0) {
                // Переключение зарядки (как в React)
                g_slave_data.car_charging_permission = !g_slave_data.car_charging_permission;
                strcpy(g_station_status, g_slave_data.car_charging_permission ? "charging" : "available");
                ESP_LOGI(TAG, "Toggled charging: %s", g_station_status);
                
            } else if (strcmp(cmd, "emergency_stop") == 0) {
                // Экстренная остановка (как в React)
                g_slave_data.car_charging_permission = false;
                g_slave_data.car_connection = false;
                strcpy(g_station_status, "emergency_stopped");
                ESP_LOGI(TAG, "Emergency stop executed");
            }
        }
        
        cJSON_Delete(json);
        
        // Возвращаем статус успеха
        cJSON *response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "station_status", g_station_status);
        
        char *response_string = cJSON_Print(response);
        
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, response_string, strlen(response_string));
        
        free(response_string);
        cJSON_Delete(response);
    }
    
    return ESP_OK;
}

// CORS обработчик для предварительных запросов
static esp_err_t cors_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// Запуск HTTP сервера с React интерфейсом
httpd_handle_t start_react_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.stack_size = 8192;
    config.server_port = 80;

    httpd_handle_t server = NULL;
    
    ESP_LOGI(TAG, "Starting React web server on port %d", config.server_port);
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Главная страница React интерфейса
        httpd_uri_t react_interface_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = react_interface_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &react_interface_uri);
        
        // Альтернативный путь для React интерфейса
        httpd_uri_t charging_station_uri = {
            .uri = "/charging-station",
            .method = HTTP_GET,
            .handler = react_interface_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &charging_station_uri);

        // API информации о станции
        httpd_uri_t api_info_uri = {
            .uri = "/api/info",
            .method = HTTP_GET,
            .handler = api_info_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_info_uri);

        // API данных (GET/POST)
        httpd_uri_t api_data_get_uri = {
            .uri = "/api/data",
            .method = HTTP_GET,
            .handler = api_data_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_data_get_uri);
        
        httpd_uri_t api_data_post_uri = {
            .uri = "/api/data",
            .method = HTTP_POST,
            .handler = api_data_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_data_post_uri);

        // CORS support
        httpd_uri_t cors_uri = {
            .uri = "/*",
            .method = HTTP_OPTIONS,
            .handler = cors_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &cors_uri);

        ESP_LOGI(TAG, "React web server started successfully");
        ESP_LOGI(TAG, "Access the interface at: http://[ESP32_IP]/");
    } else {
        ESP_LOGE(TAG, "Failed to start React web server");
    }
    
    return server;
}
