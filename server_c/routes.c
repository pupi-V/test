/**
 * Реализация HTTP маршрутов для REST API зарядных станций
 * Поддерживает все необходимые CRUD операции и интеграцию с ESP32
 */

#include "routes.h"
#include "storage.h"
#include "esp32_client.h"
#include "http_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <cjson/cJSON.h>

/**
 * Логирование HTTP запросов в стиле Express.js
 */
void log_request(const char *method, const char *url, int status_code, const char *response_data) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%I:%M:%S %p", tm_info);
    
    printf("%s [express] %s %s %d", time_str, method, url, status_code);
    
    if (response_data && strlen(response_data) > 0) {
        if (strlen(response_data) > 60) {
            printf(" :: %.60s…\n", response_data);
        } else {
            printf(" :: %s\n", response_data);
        }
    } else {
        printf("\n");
    }
}

/**
 * Отправка JSON ответа клиенту
 */
enum MHD_Result send_json_response(struct MHD_Connection *connection, int status_code, const char *json_data) {
    struct MHD_Response *response;
    enum MHD_Result ret;
    
    response = MHD_create_response_from_buffer(strlen(json_data),
                                              (void*)json_data,
                                              MHD_RESPMEM_MUST_COPY);
    
    if (!response) {
        return MHD_NO;
    }
    
    // Устанавливаем CORS заголовки
    MHD_add_response_header(response, "Content-Type", "application/json; charset=utf-8");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    
    return ret;
}

/**
 * Отправка ошибки в JSON формате
 */
enum MHD_Result send_error_response(struct MHD_Connection *connection, int status_code, const char *message) {
    cJSON *error_json = cJSON_CreateObject();
    cJSON_AddStringToObject(error_json, "message", message);
    
    char *json_string = cJSON_Print(error_json);
    enum MHD_Result result = send_json_response(connection, status_code, json_string);
    
    free(json_string);
    cJSON_Delete(error_json);
    
    return result;
}

/**
 * Извлечение ID из URL вида /api/stations/123
 */
int extract_id_from_url(const char *url, const char *prefix) {
    if (!url || !prefix) return -1;
    
    size_t prefix_len = strlen(prefix);
    if (strncmp(url, prefix, prefix_len) != 0) {
        return -1;
    }
    
    const char *id_part = url + prefix_len;
    if (*id_part == '/') {
        id_part++;
    }
    
    // Проверяем, что после ID нет дополнительных символов (кроме / для sub-routes)
    char *end_ptr;
    long id = strtol(id_part, &end_ptr, 10);
    
    if (end_ptr == id_part || id <= 0) {
        return -1;
    }
    
    // Если после ID есть символы, проверяем что это валидные sub-routes
    if (*end_ptr != '\0' && *end_ptr != '/') {
        return -1;
    }
    
    return (int)id;
}

/**
 * GET /api/stations - получение всех зарядных станций
 */
enum MHD_Result handle_get_stations(struct MHD_Connection *connection) {
    stations_array_t stations;
    
    if (storage_get_stations(&stations) != 0) {
        return send_error_response(connection, 500, "Failed to fetch stations");
    }
    
    cJSON *json_array = cJSON_CreateArray();
    if (!json_array) {
        stations_array_free(&stations);
        return send_error_response(connection, 500, "Memory allocation error");
    }
    
    for (int i = 0; i < stations.count; i++) {
        cJSON *station_json = station_to_json(&stations.stations[i]);
        if (station_json) {
            cJSON_AddItemToArray(json_array, station_json);
        }
    }
    
    char *json_string = cJSON_Print(json_array);
    enum MHD_Result result = send_json_response(connection, 200, json_string);
    
    log_request("GET", "/api/stations", 200, json_string);
    
    free(json_string);
    cJSON_Delete(json_array);
    stations_array_free(&stations);
    
    return result;
}

/**
 * GET /api/stations/:id - получение станции по ID
 */
enum MHD_Result handle_get_station(struct MHD_Connection *connection, int station_id) {
    charging_station_t station;
    
    if (storage_get_station(station_id, &station) != 0) {
        char url[64];
        snprintf(url, sizeof(url), "/api/stations/%d", station_id);
        log_request("GET", url, 404, "{\"message\":\"Station not found\"}");
        return send_error_response(connection, 404, "Station not found");
    }
    
    cJSON *station_json = station_to_json(&station);
    if (!station_json) {
        return send_error_response(connection, 500, "JSON serialization error");
    }
    
    char *json_string = cJSON_Print(station_json);
    enum MHD_Result result = send_json_response(connection, 200, json_string);
    
    char url[64];
    snprintf(url, sizeof(url), "/api/stations/%d", station_id);
    log_request("GET", url, 200, json_string);
    
    free(json_string);
    cJSON_Delete(station_json);
    
    return result;
}

/**
 * POST /api/stations - создание новой зарядной станции
 */
enum MHD_Result handle_create_station(struct MHD_Connection *connection, const char *json_data) {
    if (!json_data || strlen(json_data) == 0) {
        log_request("POST", "/api/stations", 400, "{\"message\":\"Request body required\"}");
        return send_error_response(connection, 400, "Request body required");
    }
    
    cJSON *json = cJSON_Parse(json_data);
    if (!json) {
        log_request("POST", "/api/stations", 400, "{\"message\":\"Invalid JSON\"}");
        return send_error_response(connection, 400, "Invalid JSON");
    }
    
    charging_station_t station;
    if (station_from_json(json, &station) != 0) {
        cJSON_Delete(json);
        log_request("POST", "/api/stations", 400, "{\"message\":\"Invalid station data\"}");
        return send_error_response(connection, 400, "Invalid station data");
    }
    
    if (validate_station_data(&station) != 0) {
        cJSON_Delete(json);
        log_request("POST", "/api/stations", 400, "{\"message\":\"Validation failed\"}");
        return send_error_response(connection, 400, "Validation failed");
    }
    
    int new_id;
    if (storage_create_station(&station, &new_id) != 0) {
        cJSON_Delete(json);
        log_request("POST", "/api/stations", 500, "{\"message\":\"Failed to create station\"}");
        return send_error_response(connection, 500, "Failed to create station");
    }
    
    // Получаем созданную станцию для ответа
    charging_station_t created_station;
    if (storage_get_station(new_id, &created_station) == 0) {
        cJSON *response_json = station_to_json(&created_station);
        char *response_string = cJSON_Print(response_json);
        
        enum MHD_Result result = send_json_response(connection, 201, response_string);
        log_request("POST", "/api/stations", 201, response_string);
        
        free(response_string);
        cJSON_Delete(response_json);
        cJSON_Delete(json);
        
        return result;
    }
    
    cJSON_Delete(json);
    return send_error_response(connection, 500, "Failed to retrieve created station");
}

/**
 * PATCH /api/stations/:id - обновление зарядной станции
 */
enum MHD_Result handle_update_station(struct MHD_Connection *connection, int station_id, const char *json_data) {
    if (!json_data || strlen(json_data) == 0) {
        char url[64];
        snprintf(url, sizeof(url), "/api/stations/%d", station_id);
        log_request("PATCH", url, 400, "{\"message\":\"Request body required\"}");
        return send_error_response(connection, 400, "Request body required");
    }
    
    cJSON *json = cJSON_Parse(json_data);
    if (!json) {
        char url[64];
        snprintf(url, sizeof(url), "/api/stations/%d", station_id);
        log_request("PATCH", url, 400, "{\"message\":\"Invalid JSON\"}");
        return send_error_response(connection, 400, "Invalid JSON");
    }
    
    charging_station_t updates;
    if (station_from_json(json, &updates) != 0) {
        cJSON_Delete(json);
        char url[64];
        snprintf(url, sizeof(url), "/api/stations/%d", station_id);
        log_request("PATCH", url, 400, "{\"message\":\"Invalid station data\"}");
        return send_error_response(connection, 400, "Invalid station data");
    }
    
    if (validate_update_data(&updates) != 0) {
        cJSON_Delete(json);
        char url[64];
        snprintf(url, sizeof(url), "/api/stations/%d", station_id);
        log_request("PATCH", url, 400, "{\"message\":\"Validation failed\"}");
        return send_error_response(connection, 400, "Validation failed");
    }
    
    if (storage_update_station(station_id, &updates) != 0) {
        cJSON_Delete(json);
        char url[64];
        snprintf(url, sizeof(url), "/api/stations/%d", station_id);
        log_request("PATCH", url, 404, "{\"message\":\"Station not found\"}");
        return send_error_response(connection, 404, "Station not found");
    }
    
    // Получаем обновленную станцию для ответа
    charging_station_t updated_station;
    if (storage_get_station(station_id, &updated_station) == 0) {
        cJSON *response_json = station_to_json(&updated_station);
        char *response_string = cJSON_Print(response_json);
        
        enum MHD_Result result = send_json_response(connection, 200, response_string);
        
        char url[64];
        snprintf(url, sizeof(url), "/api/stations/%d", station_id);
        log_request("PATCH", url, 200, response_string);
        
        free(response_string);
        cJSON_Delete(response_json);
        cJSON_Delete(json);
        
        return result;
    }
    
    cJSON_Delete(json);
    return send_error_response(connection, 500, "Failed to retrieve updated station");
}

/**
 * DELETE /api/stations/:id - удаление зарядной станции
 */
enum MHD_Result handle_delete_station(struct MHD_Connection *connection, int station_id) {
    if (storage_delete_station(station_id) != 0) {
        char url[64];
        snprintf(url, sizeof(url), "/api/stations/%d", station_id);
        log_request("DELETE", url, 404, "{\"message\":\"Station not found\"}");
        return send_error_response(connection, 404, "Station not found");
    }
    
    // Отправляем пустой ответ со статусом 204 (No Content)
    struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
    
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    enum MHD_Result result = MHD_queue_response(connection, 204, response);
    MHD_destroy_response(response);
    
    char url[64];
    snprintf(url, sizeof(url), "/api/stations/%d", station_id);
    log_request("DELETE", url, 204, "");
    
    return result;
}

/**
 * POST /api/board/connect - подключение к плате по ID
 */
enum MHD_Result handle_board_connect(struct MHD_Connection *connection, const char *json_data) {
    if (!json_data || strlen(json_data) == 0) {
        log_request("POST", "/api/board/connect", 400, "{\"message\":\"Request body required\"}");
        return send_error_response(connection, 400, "Request body required");
    }
    
    cJSON *json = cJSON_Parse(json_data);
    if (!json) {
        log_request("POST", "/api/board/connect", 400, "{\"message\":\"Invalid JSON\"}");
        return send_error_response(connection, 400, "Invalid JSON");
    }
    
    cJSON *board_id_json = cJSON_GetObjectItem(json, "boardId");
    if (!cJSON_IsNumber(board_id_json)) {
        cJSON_Delete(json);
        log_request("POST", "/api/board/connect", 400, "{\"message\":\"Board ID is required\"}");
        return send_error_response(connection, 400, "Board ID is required");
    }
    
    int board_id = board_id_json->valueint;
    
    charging_station_t station;
    if (storage_get_station(board_id, &station) != 0) {
        cJSON_Delete(json);
        log_request("POST", "/api/board/connect", 404, "{\"message\":\"Board not found\"}");
        return send_error_response(connection, 404, "Board not found");
    }
    
    // Создаем ответ с информацией о плате
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(response_json, "id", station.id);
    cJSON_AddStringToObject(response_json, "type", station.type);
    cJSON_AddStringToObject(response_json, "displayName", station.display_name);
    cJSON_AddStringToObject(response_json, "technicalName", station.technical_name);
    cJSON_AddStringToObject(response_json, "status", station.status);
    cJSON_AddNumberToObject(response_json, "maxPower", station.max_power);
    cJSON_AddNumberToObject(response_json, "currentPower", station.current_power);
    
    // Для slave-плат добавляем дополнительные данные
    if (strcmp(station.type, "slave") == 0) {
        cJSON_AddBoolToObject(response_json, "carConnection", station.car_connection);
        cJSON_AddBoolToObject(response_json, "carChargingPermission", station.car_charging_permission);
        cJSON_AddBoolToObject(response_json, "carError", station.car_error);
        cJSON_AddBoolToObject(response_json, "masterOnline", station.master_online);
        cJSON_AddBoolToObject(response_json, "masterChargingPermission", station.master_charging_permission);
        cJSON_AddNumberToObject(response_json, "masterAvailablePower", station.master_available_power);
        cJSON_AddNumberToObject(response_json, "voltagePhase1", station.voltage_phase1);
        cJSON_AddNumberToObject(response_json, "voltagePhase2", station.voltage_phase2);
        cJSON_AddNumberToObject(response_json, "voltagePhase3", station.voltage_phase3);
        cJSON_AddNumberToObject(response_json, "currentPhase1", station.current_phase1);
        cJSON_AddNumberToObject(response_json, "currentPhase2", station.current_phase2);
        cJSON_AddNumberToObject(response_json, "currentPhase3", station.current_phase3);
        cJSON_AddNumberToObject(response_json, "chargerPower", station.charger_power);
        cJSON_AddBoolToObject(response_json, "singlePhaseConnection", station.single_phase_connection);
        cJSON_AddBoolToObject(response_json, "powerOverconsumption", station.power_overconsumption);
        cJSON_AddBoolToObject(response_json, "fixedPower", station.fixed_power);
    }
    
    char *response_string = cJSON_Print(response_json);
    enum MHD_Result result = send_json_response(connection, 200, response_string);
    
    log_request("POST", "/api/board/connect", 200, response_string);
    
    free(response_string);
    cJSON_Delete(response_json);
    cJSON_Delete(json);
    
    return result;
}

/**
 * POST /api/esp32/scan - сканирование сети на ESP32 платы
 */
enum MHD_Result handle_esp32_scan(struct MHD_Connection *connection) {
    printf("Начинаем сканирование сети для поиска ESP32 плат...\n");
    
    esp32_board_info_t *boards = NULL;
    int board_count = 0;
    
    // Выполняем сканирование
    if (esp32_scan_network(&boards, &board_count) != 0) {
        printf("Ошибка сканирования ESP32 плат\n");
        log_request("POST", "/api/esp32/scan", 500, "{\"error\":\"Ошибка при сканировании сети\"}");
        return send_error_response(connection, 500, "Ошибка при сканировании сети");
    }
    
    printf("Сканирование завершено. Найдено плат: %d\n", board_count);
    
    // Создаем JSON ответ
    cJSON *json_array = cJSON_CreateArray();
    if (!json_array) {
        if (boards) free(boards);
        return send_error_response(connection, 500, "Memory allocation error");
    }
    
    for (int i = 0; i < board_count; i++) {
        cJSON *board_json = cJSON_CreateObject();
        cJSON_AddStringToObject(board_json, "id", boards[i].id);
        cJSON_AddStringToObject(board_json, "type", boards[i].type);
        cJSON_AddStringToObject(board_json, "ip", boards[i].ip);
        cJSON_AddStringToObject(board_json, "name", boards[i].name);
        cJSON_AddStringToObject(board_json, "status", boards[i].status);
        cJSON_AddStringToObject(board_json, "lastSeen", boards[i].last_seen);
        cJSON_AddItemToArray(json_array, board_json);
    }
    
    char *json_string = cJSON_Print(json_array);
    enum MHD_Result result = send_json_response(connection, 200, json_string);
    
    log_request("POST", "/api/esp32/scan", 200, json_string);
    
    free(json_string);
    cJSON_Delete(json_array);
    if (boards) free(boards);
    
    return result;
}

/**
 * Основной обработчик HTTP запросов
 */
enum MHD_Result route_handler(void *cls,
                             struct MHD_Connection *connection,
                             const char *url,
                             const char *method,
                             const char *version,
                             const char *upload_data,
                             size_t *upload_data_size,
                             void **con_cls) {
    
    // Обработка OPTIONS запросов для CORS
    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
        enum MHD_Result result = MHD_queue_response(connection, 200, response);
        MHD_destroy_response(response);
        return result;
    }
    
    // Обработка загрузки данных для POST/PATCH запросов
    if (*upload_data_size != 0) {
        request_data_t *req_data = *con_cls;
        
        if (!req_data) {
            req_data = malloc(sizeof(request_data_t));
            req_data->data = malloc(1024);
            req_data->size = 0;
            req_data->capacity = 1024;
            *con_cls = req_data;
        }
        
        // Расширяем буфер если нужно
        if (req_data->size + *upload_data_size >= req_data->capacity) {
            req_data->capacity = req_data->size + *upload_data_size + 1024;
            req_data->data = realloc(req_data->data, req_data->capacity);
        }
        
        memcpy(req_data->data + req_data->size, upload_data, *upload_data_size);
        req_data->size += *upload_data_size;
        req_data->data[req_data->size] = '\0';
        
        *upload_data_size = 0;
        return MHD_YES;
    }
    
    // Получаем данные запроса
    request_data_t *req_data = *con_cls;
    const char *json_data = req_data ? req_data->data : NULL;
    
    // Маршрутизация API запросов
    if (strncmp(url, "/api/", 5) == 0) {
        
        // GET /api/stations
        if (strcmp(url, "/api/stations") == 0 && strcmp(method, "GET") == 0) {
            return handle_get_stations(connection);
        }
        
        // POST /api/stations
        if (strcmp(url, "/api/stations") == 0 && strcmp(method, "POST") == 0) {
            return handle_create_station(connection, json_data);
        }
        
        // GET /api/stations/:id
        if (strncmp(url, "/api/stations/", 14) == 0 && strcmp(method, "GET") == 0) {
            int station_id = extract_id_from_url(url, "/api/stations");
            if (station_id > 0) {
                return handle_get_station(connection, station_id);
            }
        }
        
        // PATCH /api/stations/:id
        if (strncmp(url, "/api/stations/", 14) == 0 && strcmp(method, "PATCH") == 0) {
            int station_id = extract_id_from_url(url, "/api/stations");
            if (station_id > 0) {
                return handle_update_station(connection, station_id, json_data);
            }
        }
        
        // DELETE /api/stations/:id
        if (strncmp(url, "/api/stations/", 14) == 0 && strcmp(method, "DELETE") == 0) {
            int station_id = extract_id_from_url(url, "/api/stations");
            if (station_id > 0) {
                return handle_delete_station(connection, station_id);
            }
        }
        
        // POST /api/board/connect
        if (strcmp(url, "/api/board/connect") == 0 && strcmp(method, "POST") == 0) {
            return handle_board_connect(connection, json_data);
        }
        
        // POST /api/esp32/scan
        if (strcmp(url, "/api/esp32/scan") == 0 && strcmp(method, "POST") == 0) {
            return handle_esp32_scan(connection);
        }
        
        // Неизвестный API endpoint
        log_request(method, url, 404, "{\"message\":\"API endpoint not found\"}");
        return send_error_response(connection, 404, "API endpoint not found");
    }
    
    // Обслуживание статических файлов (фронтенд)
    // В продакшене здесь будет обслуживание собранных файлов React
    return send_error_response(connection, 404, "Not Found");
}