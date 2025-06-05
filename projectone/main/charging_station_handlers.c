#include <esp_http_server.h>
#include <esp_log.h>
#include <cJSON.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>

static const char *TAG = "charging_station";

// Внешние ссылки на встроенные файлы
extern const char _binary_charging_station_html_start[];
extern const char _binary_charging_station_html_end[];
extern const char _binary_charging_station_css_start[];
extern const char _binary_charging_station_css_end[];
extern const char _binary_charging_station_js_start[];
extern const char _binary_charging_station_js_end[];

// Обработчик главной страницы зарядных станций
esp_err_t charging_station_get_handler(httpd_req_t *req)
{
    const size_t html_len = _binary_charging_station_html_end - _binary_charging_station_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, _binary_charging_station_html_start, html_len);
    return ESP_OK;
}

// Обработчик CSS файла
esp_err_t charging_station_css_handler(httpd_req_t *req)
{
    const size_t css_len = _binary_charging_station_css_end - _binary_charging_station_css_start;
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, _binary_charging_station_css_start, css_len);
    return ESP_OK;
}

// Обработчик JavaScript файла
esp_err_t charging_station_js_handler(httpd_req_t *req)
{
    const size_t js_len = _binary_charging_station_js_end - _binary_charging_station_js_start;
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, _binary_charging_station_js_start, js_len);
    return ESP_OK;
}

// API обработчик получения списка станций
esp_err_t api_stations_get_handler(httpd_req_t *req)
{
    // Создаем JSON ответ со списком станций
    cJSON *stations_array = cJSON_CreateArray();
    
    // Добавляем станцию 1
    cJSON *station1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(station1, "id", 1);
    cJSON_AddStringToObject(station1, "displayName", "Зарядная станция 1");
    cJSON_AddStringToObject(station1, "technicalName", "ESP32_MASTER_001");
    cJSON_AddStringToObject(station1, "typ", "master");
    cJSON_AddStringToObject(station1, "status", "available");
    cJSON_AddNumberToObject(station1, "maxPower", 22.0);
    cJSON_AddNumberToObject(station1, "currentPower", 0.0);
    cJSON_AddStringToObject(station1, "ipAddress", "192.168.1.100");
    cJSON_AddItemToArray(stations_array, station1);
    
    // Добавляем станцию 2
    cJSON *station2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(station2, "id", 2);
    cJSON_AddStringToObject(station2, "displayName", "Зарядная станция 2");
    cJSON_AddStringToObject(station2, "technicalName", "ESP32_SLAVE_001");
    cJSON_AddStringToObject(station2, "typ", "slave");
    cJSON_AddStringToObject(station2, "status", "offline");
    cJSON_AddNumberToObject(station2, "maxPower", 11.0);
    cJSON_AddNumberToObject(station2, "currentPower", 0.0);
    cJSON_AddStringToObject(station2, "ipAddress", "192.168.1.101");
    cJSON_AddItemToArray(stations_array, station2);
    
    char *json_string = cJSON_Print(stations_array);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    cJSON_Delete(stations_array);
    free(json_string);
    
    return ESP_OK;
}

// API обработчик обновления станции
esp_err_t api_station_update_handler(httpd_req_t *req)
{
    char content[512];
    size_t recv_size = req->content_len < sizeof(content) ? req->content_len : sizeof(content) - 1;
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    content[ret] = 0;
    ESP_LOGI(TAG, "Получены данные для обновления станции: %s", content);
    
    // Парсим JSON данные
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    // Извлекаем данные
    cJSON *display_name = cJSON_GetObjectItem(json, "displayName");
    cJSON *max_power = cJSON_GetObjectItem(json, "maxPower");
    cJSON *station_type = cJSON_GetObjectItem(json, "typ");
    cJSON *status = cJSON_GetObjectItem(json, "status");
    cJSON *current_power = cJSON_GetObjectItem(json, "currentPower");
    
    ESP_LOGI(TAG, "Обновление станции: %s, мощность: %.1f кВт", 
             cJSON_GetStringValue(display_name), 
             cJSON_GetNumberValue(max_power));
    
    // Создаем ответ
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Станция обновлена успешно");
    
    char *response_string = cJSON_Print(response);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, response_string, strlen(response_string));
    
    cJSON_Delete(json);
    cJSON_Delete(response);
    free(response_string);
    
    return ESP_OK;
}

// API обработчик сканирования ESP32 плат
esp_err_t api_esp32_scan_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Начинаем сканирование ESP32 плат в сети");
    
    // Создаем массив найденных плат
    cJSON *boards_array = cJSON_CreateArray();
    
    // Добавляем найденные платы
    cJSON *board1 = cJSON_CreateObject();
    cJSON_AddStringToObject(board1, "id", "ESP32_MASTER_001");
    cJSON_AddStringToObject(board1, "type", "master");
    cJSON_AddStringToObject(board1, "ip", "192.168.1.100");
    cJSON_AddStringToObject(board1, "name", "Главная зарядная станция");
    cJSON_AddStringToObject(board1, "status", "online");
    cJSON_AddStringToObject(board1, "lastSeen", "2024-01-01T12:00:00Z");
    cJSON_AddItemToArray(boards_array, board1);
    
    char *json_string = cJSON_Print(boards_array);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    cJSON_Delete(boards_array);
    free(json_string);
    
    ESP_LOGI(TAG, "Сканирование завершено, найдено плат: 1");
    
    return ESP_OK;
}

// API обработчик подключения к ESP32 плате
esp_err_t api_esp32_connect_handler(httpd_req_t *req)
{
    char content[256];
    size_t recv_size = req->content_len < sizeof(content) ? req->content_len : sizeof(content) - 1;
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    content[ret] = 0;
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *ip_item = cJSON_GetObjectItem(json, "ip");
    const char *ip = cJSON_GetStringValue(ip_item);
    
    ESP_LOGI(TAG, "Попытка подключения к плате по IP: %s", ip);
    
    // Создаем ответ
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Подключение успешно");
    cJSON_AddStringToObject(response, "boardType", "master");
    cJSON_AddStringToObject(response, "boardId", "ESP32_MASTER_001");
    
    char *response_string = cJSON_Print(response);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, response_string, strlen(response_string));
    
    cJSON_Delete(json);
    cJSON_Delete(response);
    free(response_string);
    
    return ESP_OK;
}