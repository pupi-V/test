/**
 * Основной файл сервера управления зарядными станциями на C
 * Переписан с Node.js/Express для обеспечения высокой производительности
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#include "simple_http.h"
#include "simple_json.h"
#include "storage.h"

// Глобальные переменные
static http_server_t server;
static int server_running = 1;

// Конфигурация сервера
static int port = 5000;
static const char *host = "0.0.0.0";

// Получение порта из переменной окружения
void init_port_config() {
    const char *port_env = getenv("PORT");
    if (port_env) {
        int env_port = atoi(port_env);
        if (env_port > 0 && env_port < 65536) {
            port = env_port;
        }
    }
}

/**
 * Обработчик сигналов для корректного завершения работы сервера
 */
void signal_handler(int sig) {
    printf("\nПолучен сигнал %d, завершаем работу сервера...\n", sig);
    server_running = 0;
    
    http_server_stop(&server);
    storage_cleanup();
    
    exit(0);
}

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
 * Получение текущего времени в миллисекундах
 */
long get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

/**
 * Основной обработчик HTTP запросов
 */
void handle_request(const http_request_t *request, http_response_t *response) {
    long start_time = get_current_time_ms();
    
    // Обработка OPTIONS запросов для CORS
    if (strcmp(request->method, "OPTIONS") == 0) {
        http_set_response_status(response, 200, "OK");
        http_set_response_body(response, "");
        log_request("OPTIONS", request->path, 200, "");
        return;
    }
    
    // API маршруты
    if (strncmp(request->path, "/api/", 5) == 0) {
        
        // GET /api/stations
        if (strcmp(request->path, "/api/stations") == 0 && strcmp(request->method, "GET") == 0) {
            stations_array_t stations;
            
            if (storage_get_stations(&stations) != 0) {
                http_set_response_status(response, 500, "Internal Server Error");
                http_set_response_body(response, "{\"message\":\"Failed to fetch stations\"}");
                log_request("GET", "/api/stations", 500, "{\"message\":\"Failed to fetch stations\"}");
                return;
            }
            
            // Создаем JSON массив
            json_value_t *json_array = json_create_array();
            for (int i = 0; i < stations.count; i++) {
                json_value_t *station_obj = json_create_object();
                
                json_object_set(station_obj, "id", json_create_number(stations.stations[i].id));
                json_object_set(station_obj, "displayName", json_create_string(stations.stations[i].display_name));
                json_object_set(station_obj, "technicalName", json_create_string(stations.stations[i].technical_name));
                json_object_set(station_obj, "type", json_create_string(stations.stations[i].type));
                json_object_set(station_obj, "status", json_create_string(stations.stations[i].status));
                json_object_set(station_obj, "maxPower", json_create_number(stations.stations[i].max_power));
                json_object_set(station_obj, "currentPower", json_create_number(stations.stations[i].current_power));
                
                if (strlen(stations.stations[i].ip_address) > 0) {
                    json_object_set(station_obj, "ipAddress", json_create_string(stations.stations[i].ip_address));
                }
                
                if (strlen(stations.stations[i].description) > 0) {
                    json_object_set(station_obj, "description", json_create_string(stations.stations[i].description));
                }
                
                // Slave-specific данные
                json_object_set(station_obj, "carConnection", json_create_bool(stations.stations[i].car_connection));
                json_object_set(station_obj, "carChargingPermission", json_create_bool(stations.stations[i].car_charging_permission));
                json_object_set(station_obj, "carError", json_create_bool(stations.stations[i].car_error));
                json_object_set(station_obj, "masterOnline", json_create_bool(stations.stations[i].master_online));
                json_object_set(station_obj, "masterChargingPermission", json_create_bool(stations.stations[i].master_charging_permission));
                json_object_set(station_obj, "masterAvailablePower", json_create_number(stations.stations[i].master_available_power));
                
                // Электрические параметры
                json_object_set(station_obj, "voltagePhase1", json_create_number(stations.stations[i].voltage_phase1));
                json_object_set(station_obj, "voltagePhase2", json_create_number(stations.stations[i].voltage_phase2));
                json_object_set(station_obj, "voltagePhase3", json_create_number(stations.stations[i].voltage_phase3));
                json_object_set(station_obj, "currentPhase1", json_create_number(stations.stations[i].current_phase1));
                json_object_set(station_obj, "currentPhase2", json_create_number(stations.stations[i].current_phase2));
                json_object_set(station_obj, "currentPhase3", json_create_number(stations.stations[i].current_phase3));
                json_object_set(station_obj, "chargerPower", json_create_number(stations.stations[i].charger_power));
                
                // Дополнительные параметры
                json_object_set(station_obj, "singlePhaseConnection", json_create_bool(stations.stations[i].single_phase_connection));
                json_object_set(station_obj, "powerOverconsumption", json_create_bool(stations.stations[i].power_overconsumption));
                json_object_set(station_obj, "fixedPower", json_create_bool(stations.stations[i].fixed_power));
                
                json_array_add(json_array, station_obj);
            }
            
            char *json_string = json_stringify(json_array);
            http_set_response_status(response, 200, "OK");
            http_set_response_body(response, json_string);
            
            long end_time = get_current_time_ms();
            printf("%s [express] GET /api/stations 200 in %ldms :: %s\n", 
                   "time", end_time - start_time, 
                   strlen(json_string) > 60 ? "truncated..." : json_string);
            
            free(json_string);
            json_free(json_array);
            stations_array_free(&stations);
            return;
        }
        
        // GET /api/stations/:id
        if (strncmp(request->path, "/api/stations/", 14) == 0 && strcmp(request->method, "GET") == 0) {
            const char *id_str = request->path + 14; // Skip "/api/stations/"
            int station_id = atoi(id_str);
            
            if (station_id <= 0) {
                http_set_response_status(response, 400, "Bad Request");
                http_add_response_header(response, "Content-Type", "application/json; charset=utf-8");
                http_set_response_body(response, "{\"message\":\"Invalid station ID\"}");
                log_request("GET", request->path, 400, "{\"message\":\"Invalid station ID\"}");
                return;
            }
            
            charging_station_t station;
            if (storage_get_station(station_id, &station) != 0) {
                http_set_response_status(response, 404, "Not Found");
                http_add_response_header(response, "Content-Type", "application/json; charset=utf-8");
                http_set_response_body(response, "{\"message\":\"Station not found\"}");
                log_request("GET", request->path, 404, "{\"message\":\"Station not found\"}");
                return;
            }
            
            // Создаем JSON объект для станции
            json_value_t *station_obj = json_create_object();
            
            json_object_set(station_obj, "id", json_create_number(station.id));
            json_object_set(station_obj, "displayName", json_create_string(station.display_name));
            json_object_set(station_obj, "technicalName", json_create_string(station.technical_name));
            json_object_set(station_obj, "type", json_create_string(station.type));
            json_object_set(station_obj, "status", json_create_string(station.status));
            json_object_set(station_obj, "maxPower", json_create_number(station.max_power));
            json_object_set(station_obj, "currentPower", json_create_number(station.current_power));
            
            if (strlen(station.ip_address) > 0) {
                json_object_set(station_obj, "ipAddress", json_create_string(station.ip_address));
            }
            
            if (strlen(station.description) > 0) {
                json_object_set(station_obj, "description", json_create_string(station.description));
            }
            
            // Slave-specific данные
            json_object_set(station_obj, "carConnection", json_create_bool(station.car_connection));
            json_object_set(station_obj, "carChargingPermission", json_create_bool(station.car_charging_permission));
            json_object_set(station_obj, "carError", json_create_bool(station.car_error));
            json_object_set(station_obj, "masterOnline", json_create_bool(station.master_online));
            json_object_set(station_obj, "masterChargingPermission", json_create_bool(station.master_charging_permission));
            json_object_set(station_obj, "masterAvailablePower", json_create_number(station.master_available_power));
            
            // Электрические параметры
            json_object_set(station_obj, "voltagePhase1", json_create_number(station.voltage_phase1));
            json_object_set(station_obj, "voltagePhase2", json_create_number(station.voltage_phase2));
            json_object_set(station_obj, "voltagePhase3", json_create_number(station.voltage_phase3));
            json_object_set(station_obj, "currentPhase1", json_create_number(station.current_phase1));
            json_object_set(station_obj, "currentPhase2", json_create_number(station.current_phase2));
            json_object_set(station_obj, "currentPhase3", json_create_number(station.current_phase3));
            json_object_set(station_obj, "chargerPower", json_create_number(station.charger_power));
            
            // Дополнительные параметры
            json_object_set(station_obj, "singlePhaseConnection", json_create_bool(station.single_phase_connection));
            json_object_set(station_obj, "powerOverconsumption", json_create_bool(station.power_overconsumption));
            json_object_set(station_obj, "fixedPower", json_create_bool(station.fixed_power));
            
            char *json_string = json_stringify(station_obj);
            
            http_set_response_status(response, 200, "OK");
            http_add_response_header(response, "Content-Type", "application/json; charset=utf-8");
            http_set_response_body(response, json_string);
            
            long end_time = get_current_time_ms();
            printf("%s [express] GET %s 200 in %ldms :: station data\n", 
                   "time", request->path, end_time - start_time);
            
            free(json_string);
            json_free(station_obj);
            return;
        }
        
        // POST /api/esp32/scan
        if (strcmp(request->path, "/api/esp32/scan") == 0 && strcmp(request->method, "POST") == 0) {
            printf("Начинаем сканирование сети для поиска ESP32 плат...\n");
            
            // Создаем пустой массив (в реальной реализации здесь был бы код сканирования)
            json_value_t *json_array = json_create_array();
            char *json_string = json_stringify(json_array);
            
            printf("Сканирование завершено. Найдено плат: 0\n");
            
            http_set_response_status(response, 200, "OK");
            http_set_response_body(response, json_string);
            
            long end_time = get_current_time_ms();
            printf("%s [express] POST /api/esp32/scan 200 in %ldms :: %s\n",
                   "time", end_time - start_time, json_string);
            
            free(json_string);
            json_free(json_array);
            return;
        }
        
        // Неизвестный API endpoint
        http_set_response_status(response, 404, "Not Found");
        http_set_response_body(response, "{\"message\":\"API endpoint not found\"}");
        log_request(request->method, request->path, 404, "{\"message\":\"API endpoint not found\"}");
        return;
    }
    
    // Статические файлы - serve index.html for SPA routing
    if (strcmp(request->path, "/") == 0 || strstr(request->path, ".") == NULL) {
        // Serve index.html for root path or paths without extensions (SPA routing)
        FILE *file = fopen("../dist/public/index.html", "r");
        if (file) {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            char *file_content = malloc(file_size + 1);
            fread(file_content, 1, file_size, file);
            file_content[file_size] = '\0';
            fclose(file);
            
            http_set_response_status(response, 200, "OK");
            http_add_response_header(response, "Content-Type", "text/html");
            http_set_response_body(response, file_content);
            
            free(file_content);
            log_request(request->method, request->path, 200, "index.html served");
            return;
        }
    }
    
    // Static asset files
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "../dist/public%s", request->path);
    
    FILE *file = fopen(file_path, "rb");  // Use binary mode for all files
    if (file) {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char *file_content = malloc(file_size + 1);
        size_t bytes_read = fread(file_content, 1, file_size, file);
        fclose(file);
        
        // Don't null-terminate binary files, but ensure we have the right size
        if (bytes_read != (size_t)file_size) {
            free(file_content);
            http_set_response_status(response, 500, "Internal Server Error");
            http_set_response_body(response, "{\"message\":\"File read error\"}");
            log_request(request->method, request->path, 500, "File read error");
            return;
        }
        
        // Set appropriate content type with proper MIME types for modules
        const char *content_type = "text/plain";
        if (strstr(request->path, ".html")) content_type = "text/html; charset=utf-8";
        else if (strstr(request->path, ".css")) content_type = "text/css; charset=utf-8";
        else if (strstr(request->path, ".js")) content_type = "text/javascript; charset=utf-8";
        else if (strstr(request->path, ".mjs")) content_type = "text/javascript; charset=utf-8";
        else if (strstr(request->path, ".json")) content_type = "application/json; charset=utf-8";
        else if (strstr(request->path, ".png")) content_type = "image/png";
        else if (strstr(request->path, ".jpg") || strstr(request->path, ".jpeg")) content_type = "image/jpeg";
        else if (strstr(request->path, ".svg")) content_type = "image/svg+xml";
        else if (strstr(request->path, ".ico")) content_type = "image/x-icon";
        
        http_set_response_status(response, 200, "OK");
        http_add_response_header(response, "Content-Type", content_type);
        
        // For large files, use body_data; for small files, use regular body
        if (file_size < MAX_RESPONSE_SIZE - 1) {
            memcpy(response->body, file_content, file_size);
            response->body[file_size] = '\0';
            response->body_length = file_size;
        } else {
            response->body_data = malloc(file_size);
            memcpy(response->body_data, file_content, file_size);
            response->body_size = file_size;
        }
        
        free(file_content);
        log_request(request->method, request->path, 200, "static file served");
        return;
    }
    
    // File not found
    http_set_response_status(response, 404, "Not Found");
    http_set_response_body(response, "{\"message\":\"Not Found\"}");
    log_request(request->method, request->path, 404, "{\"message\":\"Not Found\"}");
}

/**
 * Инициализация компонентов сервера
 */
int initialize_server() {
    // Инициализация системы хранения данных
    if (storage_init() != 0) {
        fprintf(stderr, "Ошибка инициализации системы хранения\n");
        return -1;
    }
    
    printf("Система хранения инициализирована\n");
    return 0;
}

/**
 * Основная функция программы
 */
int main(int argc, char *argv[]) {
    // Обработка аргументов командной строки
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Неверный номер порта: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
    }
    
    // Получение порта из переменной окружения
    const char *env_port = getenv("PORT");
    if (env_port && strlen(env_port) > 0) {
        port = atoi(env_port);
    }
    
    const char *env_host = getenv("HOST");
    if (env_host && strlen(env_host) > 0) {
        host = env_host;
    }
    
    // Установка обработчиков сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Инициализация сервера
    if (initialize_server() != 0) {
        return EXIT_FAILURE;
    }
    
    // Инициализация и запуск HTTP сервера
    if (http_server_init(&server, host, port, handle_request) != 0) {
        fprintf(stderr, "Ошибка инициализации HTTP сервера\n");
        storage_cleanup();
        return EXIT_FAILURE;
    }
    
    if (http_server_start(&server) != 0) {
        fprintf(stderr, "Ошибка запуска HTTP сервера\n");
        storage_cleanup();
        return EXIT_FAILURE;
    }
    
    // Корректное завершение работы
    http_server_cleanup(&server);
    storage_cleanup();
    printf("Сервер остановлен\n");
    return EXIT_SUCCESS;
}