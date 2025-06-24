/**
 * Заголовочный файл для работы с ESP32 платами
 * Содержит функции сканирования сети и взаимодействия с контроллерами
 */

#ifndef ESP32_CLIENT_H
#define ESP32_CLIENT_H

#include <curl/curl.h>

// Максимальные размеры строк для ESP32 данных
#define ESP32_MAX_STRING 256
#define ESP32_MAX_IP 16

/**
 * Структура информации о ESP32 плате
 */
typedef struct {
    char id[ESP32_MAX_STRING];
    char type[32]; // "master" или "slave"
    char ip[ESP32_MAX_IP];
    char name[ESP32_MAX_STRING];
    char technical_name[ESP32_MAX_STRING];
    float max_power;
    char status[32]; // "online" или "offline"
    char last_seen[64];
} esp32_board_info_t;

/**
 * Структура для хранения ответа HTTP запроса
 */
typedef struct {
    char *data;
    size_t size;
} http_response_t;

/**
 * Функции для работы с ESP32 платами
 */

// Сканирование локальной сети для поиска ESP32 плат
int esp32_scan_network(esp32_board_info_t **boards, int *count);

// Подключение к конкретной ESP32 плате по IP
int esp32_connect_to_board(const char *ip, const char *expected_type, esp32_board_info_t *board_info);

// Отправка данных на ESP32 плату
int esp32_send_data(const char *ip, const char *json_data);

// Получение данных с ESP32 платы
int esp32_get_data(const char *ip, char **response_data);

// Проверка доступности ESP32 платы
int esp32_ping_board(const char *ip);

// Проверка является ли устройство ESP32 зарядной станцией
int esp32_check_charging_board(const char *ip, esp32_board_info_t *board_info);

/**
 * Вспомогательные функции
 */

// Получение информации о локальной сети
int get_local_network_info(char *network_base, char *subnet_mask);

// Callback функция для обработки ответа CURL
size_t curl_write_callback(void *contents, size_t size, size_t nmemb, http_response_t *response);

// Освобождение памяти HTTP ответа
void free_http_response(http_response_t *response);

// Парсинг JSON ответа от ESP32
int parse_esp32_response(const char *json_data, esp32_board_info_t *board_info);

#endif // ESP32_CLIENT_H