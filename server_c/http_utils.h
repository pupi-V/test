/**
 * Заголовочный файл для HTTP утилит
 * Содержит вспомогательные функции для работы с HTTP запросами и ответами
 */

#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <microhttpd.h>

/**
 * Структура для хранения MIME типов
 */
typedef struct {
    const char *extension;
    const char *mime_type;
} mime_type_t;

/**
 * Функции для работы с HTTP
 */

// Определение MIME типа по расширению файла
const char* get_mime_type(const char *filename);

// Обслуживание статических файлов
enum MHD_Result serve_static_file(struct MHD_Connection *connection, const char *filename);

// Установка CORS заголовков
void set_cors_headers(struct MHD_Response *response);

// Логирование HTTP запросов с временными метками
void log_http_request(const char *method, const char *url, int status_code, 
                     const char *response_data, long response_time_ms);

// Получение текущего времени в миллисекундах
long get_current_time_ms(void);

// Парсинг URL параметров
int parse_url_params(const char *url, const char *param_name, char *param_value, size_t max_len);

// Валидация JSON данных
int validate_json_string(const char *json_str);

// Создание JSON ответа с сообщением об ошибке
char* create_error_json(const char *message);

// Создание JSON ответа с успешным статусом
char* create_success_json(const char *message, const char *data);

#endif // HTTP_UTILS_H