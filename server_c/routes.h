/**
 * Заголовочный файл для обработки HTTP маршрутов
 * Содержит определения функций для работы с REST API
 */

#ifndef ROUTES_H
#define ROUTES_H

#include <microhttpd.h>

/**
 * Структура для передачи данных между обработчиками
 */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} request_data_t;

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
                             void **con_cls);

/**
 * Обработчики для различных API endpoints
 */

// GET /api/stations - получить все станции
enum MHD_Result handle_get_stations(struct MHD_Connection *connection);

// GET /api/stations/:id - получить станцию по ID
enum MHD_Result handle_get_station(struct MHD_Connection *connection, int station_id);

// POST /api/stations - создать новую станцию
enum MHD_Result handle_create_station(struct MHD_Connection *connection, const char *json_data);

// PATCH /api/stations/:id - обновить станцию
enum MHD_Result handle_update_station(struct MHD_Connection *connection, int station_id, const char *json_data);

// DELETE /api/stations/:id - удалить станцию
enum MHD_Result handle_delete_station(struct MHD_Connection *connection, int station_id);

// POST /api/board/connect - подключение к плате
enum MHD_Result handle_board_connect(struct MHD_Connection *connection, const char *json_data);

// POST /api/esp32/scan - сканирование ESP32 плат
enum MHD_Result handle_esp32_scan(struct MHD_Connection *connection);

// POST /api/esp32/connect - подключение к ESP32 плате
enum MHD_Result handle_esp32_connect(struct MHD_Connection *connection, const char *json_data);

// POST /api/esp32/:id/sync - синхронизация с ESP32
enum MHD_Result handle_esp32_sync(struct MHD_Connection *connection, int station_id);

/**
 * Вспомогательные функции
 */

// Извлечение ID из URL
int extract_id_from_url(const char *url, const char *prefix);

// Создание JSON ответа с ошибкой
enum MHD_Result send_error_response(struct MHD_Connection *connection, int status_code, const char *message);

// Создание успешного JSON ответа
enum MHD_Result send_json_response(struct MHD_Connection *connection, int status_code, const char *json_data);

// Логирование HTTP запросов
void log_request(const char *method, const char *url, int status_code, const char *response_data);

#endif // ROUTES_H