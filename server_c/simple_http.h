/**
 * Простой HTTP сервер для системы зарядных станций
 * Заменяет libmicrohttpd для совместимости с Nix окружением
 */

#ifndef SIMPLE_HTTP_H
#define SIMPLE_HTTP_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_REQUEST_SIZE 8192
#define MAX_RESPONSE_SIZE 65536
#define MAX_CONNECTIONS 100

/**
 * Структура HTTP запроса
 */
typedef struct {
    char method[16];
    char path[512];
    char version[16];
    char body[MAX_REQUEST_SIZE];
    int content_length;
} http_request_t;

/**
 * Структура HTTP ответа
 */
typedef struct {
    int status_code;
    char headers[1024];
    char body[MAX_RESPONSE_SIZE];
    int body_length;
} http_response_t;

/**
 * Обработчик HTTP запросов
 */
typedef void (*request_handler_t)(const http_request_t *request, http_response_t *response);

/**
 * Структура HTTP сервера
 */
typedef struct {
    int socket_fd;
    int port;
    const char *host;
    request_handler_t handler;
    int running;
} http_server_t;

// Функции HTTP сервера
int http_server_init(http_server_t *server, const char *host, int port, request_handler_t handler);
int http_server_start(http_server_t *server);
void http_server_stop(http_server_t *server);
void http_server_cleanup(http_server_t *server);

// Парсинг HTTP запроса
int http_parse_request(const char *raw_request, http_request_t *request);

// Формирование HTTP ответа
void http_set_response_status(http_response_t *response, int status_code, const char *status_text);
void http_add_response_header(http_response_t *response, const char *name, const char *value);
void http_set_response_body(http_response_t *response, const char *body);
char* http_format_response(const http_response_t *response);

// URL декодирование
void url_decode(char *dst, const char *src);

#endif // SIMPLE_HTTP_H