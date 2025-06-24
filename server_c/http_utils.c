/**
 * Реализация HTTP утилит
 * Вспомогательные функции для работы с веб-сервером
 */

#include "http_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <cjson/cJSON.h>

/**
 * Таблица MIME типов для статических файлов
 */
static const mime_type_t mime_types[] = {
    {".html", "text/html; charset=utf-8"},
    {".htm", "text/html; charset=utf-8"},
    {".css", "text/css; charset=utf-8"},
    {".js", "application/javascript; charset=utf-8"},
    {".json", "application/json; charset=utf-8"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".txt", "text/plain; charset=utf-8"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {NULL, NULL}
};

/**
 * Получение текущего времени в миллисекундах
 */
long get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

/**
 * Определение MIME типа по расширению файла
 */
const char* get_mime_type(const char *filename) {
    if (!filename) {
        return "text/plain";
    }
    
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return "text/plain";
    }
    
    for (int i = 0; mime_types[i].extension != NULL; i++) {
        if (strcasecmp(ext, mime_types[i].extension) == 0) {
            return mime_types[i].mime_type;
        }
    }
    
    return "application/octet-stream";
}

/**
 * Установка CORS заголовков для ответа
 */
void set_cors_headers(struct MHD_Response *response) {
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", 
                           "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", 
                           "Content-Type, Authorization, X-Requested-With");
    MHD_add_response_header(response, "Access-Control-Max-Age", "86400");
}

/**
 * Обслуживание статических файлов
 */
enum MHD_Result serve_static_file(struct MHD_Connection *connection, const char *filename) {
    FILE *file;
    struct stat file_stat;
    struct MHD_Response *response;
    enum MHD_Result ret;
    
    // Проверяем существование файла
    if (stat(filename, &file_stat) != 0) {
        return MHD_NO; // Файл не найден
    }
    
    // Открываем файл для чтения
    file = fopen(filename, "rb");
    if (!file) {
        return MHD_NO;
    }
    
    // Создаем ответ с содержимым файла
    response = MHD_create_response_from_fd(file_stat.st_size, fileno(file));
    if (!response) {
        fclose(file);
        return MHD_NO;
    }
    
    // Устанавливаем правильный MIME тип
    const char *mime_type = get_mime_type(filename);
    MHD_add_response_header(response, "Content-Type", mime_type);
    
    // Устанавливаем CORS заголовки
    set_cors_headers(response);
    
    // Отправляем ответ
    ret = MHD_queue_response(connection, 200, response);
    MHD_destroy_response(response);
    
    return ret;
}

/**
 * Логирование HTTP запросов с временными метками
 */
void log_http_request(const char *method, const char *url, int status_code, 
                     const char *response_data, long response_time_ms) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%I:%M:%S %p", tm_info);
    
    printf("%s [express] %s %s %d in %ldms", 
           time_str, method, url, status_code, response_time_ms);
    
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
 * Парсинг URL параметров
 */
int parse_url_params(const char *url, const char *param_name, char *param_value, size_t max_len) {
    if (!url || !param_name || !param_value) {
        return -1;
    }
    
    const char *query_start = strchr(url, '?');
    if (!query_start) {
        return -1; // Нет параметров
    }
    
    query_start++; // Пропускаем '?'
    
    char *query_copy = strdup(query_start);
    if (!query_copy) {
        return -1;
    }
    
    char *param = strtok(query_copy, "&");
    while (param) {
        char *equals = strchr(param, '=');
        if (equals) {
            *equals = '\0';
            if (strcmp(param, param_name) == 0) {
                strncpy(param_value, equals + 1, max_len - 1);
                param_value[max_len - 1] = '\0';
                free(query_copy);
                return 0;
            }
        }
        param = strtok(NULL, "&");
    }
    
    free(query_copy);
    return -1; // Параметр не найден
}

/**
 * Валидация JSON данных
 */
int validate_json_string(const char *json_str) {
    if (!json_str || strlen(json_str) == 0) {
        return -1;
    }
    
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        return -1;
    }
    
    cJSON_Delete(json);
    return 0;
}

/**
 * Создание JSON ответа с сообщением об ошибке
 */
char* create_error_json(const char *message) {
    cJSON *error_json = cJSON_CreateObject();
    if (!error_json) {
        return NULL;
    }
    
    cJSON_AddStringToObject(error_json, "message", message ? message : "Unknown error");
    
    char *json_string = cJSON_Print(error_json);
    cJSON_Delete(error_json);
    
    return json_string;
}

/**
 * Создание JSON ответа с успешным статусом
 */
char* create_success_json(const char *message, const char *data) {
    cJSON *success_json = cJSON_CreateObject();
    if (!success_json) {
        return NULL;
    }
    
    if (message) {
        cJSON_AddStringToObject(success_json, "message", message);
    }
    
    if (data) {
        cJSON *data_json = cJSON_Parse(data);
        if (data_json) {
            cJSON_AddItemToObject(success_json, "data", data_json);
        } else {
            cJSON_AddStringToObject(success_json, "data", data);
        }
    }
    
    char *json_string = cJSON_Print(success_json);
    cJSON_Delete(success_json);
    
    return json_string;
}