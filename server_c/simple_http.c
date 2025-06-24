/**
 * Реализация простого HTTP сервера
 * Замена libmicrohttpd для работы в Nix окружении
 */

#include "simple_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

/**
 * Структура для передачи данных в поток обработки соединения
 */
typedef struct {
    int client_fd;
    http_server_t *server;
} connection_data_t;

/**
 * Инициализация HTTP сервера
 */
int http_server_init(http_server_t *server, const char *host, int port, request_handler_t handler) {
    if (!server || !handler) {
        return -1;
    }
    
    memset(server, 0, sizeof(http_server_t));
    server->port = port;
    server->host = host;
    server->handler = handler;
    server->running = 0;
    server->socket_fd = -1;
    
    return 0;
}

/**
 * Парсинг HTTP запроса
 */
int http_parse_request(const char *raw_request, http_request_t *request) {
    if (!raw_request || !request) {
        return -1;
    }
    
    memset(request, 0, sizeof(http_request_t));
    
    // Парсим первую строку запроса
    char *line_end = strstr(raw_request, "\r\n");
    if (!line_end) {
        line_end = strstr(raw_request, "\n");
    }
    
    if (!line_end) {
        return -1;
    }
    
    // Копируем первую строку
    int line_length = line_end - raw_request;
    char first_line[512];
    strncpy(first_line, raw_request, line_length);
    first_line[line_length] = '\0';
    
    // Разбираем метод, путь и версию
    char *token = strtok(first_line, " ");
    if (token) {
        strncpy(request->method, token, sizeof(request->method) - 1);
    }
    
    token = strtok(NULL, " ");
    if (token) {
        strncpy(request->path, token, sizeof(request->path) - 1);
    }
    
    token = strtok(NULL, " ");
    if (token) {
        strncpy(request->version, token, sizeof(request->version) - 1);
    }
    
    // Ищем тело запроса (после пустой строки)
    char *body_start = strstr(raw_request, "\r\n\r\n");
    if (!body_start) {
        body_start = strstr(raw_request, "\n\n");
        if (body_start) {
            body_start += 2;
        }
    } else {
        body_start += 4;
    }
    
    if (body_start) {
        int body_length = strlen(body_start);
        if (body_length > 0 && body_length < MAX_REQUEST_SIZE - 1) {
            strcpy(request->body, body_start);
            request->content_length = body_length;
        }
    }
    
    return 0;
}

/**
 * Установка статуса ответа
 */
void http_set_response_status(http_response_t *response, int status_code, const char *status_text) {
    if (!response) return;
    
    response->status_code = status_code;
    
    // Добавляем стандартные заголовки CORS
    snprintf(response->headers, sizeof(response->headers),
        "HTTP/1.1 %d %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Content-Type: application/json; charset=utf-8\r\n",
        status_code, status_text ? status_text : "OK");
}

/**
 * Добавление заголовка к ответу
 */
void http_add_response_header(http_response_t *response, const char *name, const char *value) {
    if (!response || !name || !value) return;
    
    char header_line[256];
    snprintf(header_line, sizeof(header_line), "%s: %s\r\n", name, value);
    strncat(response->headers, header_line, sizeof(response->headers) - strlen(response->headers) - 1);
}

/**
 * Установка тела ответа
 */
void http_set_response_body(http_response_t *response, const char *body) {
    if (!response || !body) return;
    
    int body_len = strlen(body);
    if (body_len < MAX_RESPONSE_SIZE - 1) {
        strcpy(response->body, body);
        response->body_length = body_len;
    }
}

/**
 * Формирование полного HTTP ответа
 */
char* http_format_response(const http_response_t *response) {
    if (!response) return NULL;
    
    int total_size = strlen(response->headers) + response->body_length + 100;
    char *full_response = malloc(total_size);
    if (!full_response) return NULL;
    
    snprintf(full_response, total_size,
        "%sContent-Length: %d\r\n\r\n%s",
        response->headers,
        response->body_length,
        response->body);
    
    return full_response;
}

/**
 * URL декодирование
 */
void url_decode(char *dst, const char *src) {
    char *p = dst;
    char code[3];
    
    while (*src) {
        if (*src == '%') {
            memcpy(code, src + 1, 2);
            code[2] = '\0';
            *p++ = (char)strtol(code, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            *p++ = ' ';
            src++;
        } else {
            *p++ = *src++;
        }
    }
    *p = '\0';
}

/**
 * Обработка клиентского соединения в отдельном потоке
 */
void* handle_connection(void *arg) {
    connection_data_t *conn_data = (connection_data_t*)arg;
    char buffer[MAX_REQUEST_SIZE];
    
    // Читаем запрос
    ssize_t bytes_read = recv(conn_data->client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        
        // Парсим запрос
        http_request_t request;
        if (http_parse_request(buffer, &request) == 0) {
            // Создаем ответ
            http_response_t response;
            memset(&response, 0, sizeof(response));
            
            // Вызываем обработчик
            conn_data->server->handler(&request, &response);
            
            // Отправляем ответ
            char *full_response = http_format_response(&response);
            if (full_response) {
                send(conn_data->client_fd, full_response, strlen(full_response), 0);
                free(full_response);
            }
        }
    }
    
    close(conn_data->client_fd);
    free(conn_data);
    return NULL;
}

/**
 * Запуск HTTP сервера
 */
int http_server_start(http_server_t *server) {
    if (!server) {
        return -1;
    }
    
    // Создаем сокет
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        perror("Ошибка создания сокета");
        return -1;
    }
    
    // Настраиваем сокет для переиспользования адреса
    int opt = 1;
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Ошибка настройки сокета");
        close(server->socket_fd);
        return -1;
    }
    
    // Настраиваем адрес сервера
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server->port);
    
    if (strcmp(server->host, "0.0.0.0") == 0) {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, server->host, &server_addr.sin_addr) <= 0) {
            fprintf(stderr, "Неверный IP адрес: %s\n", server->host);
            close(server->socket_fd);
            return -1;
        }
    }
    
    // Привязываем сокет к адресу
    if (bind(server->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка привязки сокета");
        close(server->socket_fd);
        return -1;
    }
    
    // Начинаем прослушивание
    if (listen(server->socket_fd, MAX_CONNECTIONS) < 0) {
        perror("Ошибка прослушивания");
        close(server->socket_fd);
        return -1;
    }
    
    server->running = 1;
    printf("🚀 Запуск системы управления зарядными станциями...\n");
    printf("📍 Режим: разработка\n");
    printf("🌐 Сервер: http://%s:%d\n", server->host, server->port);
    printf("💻 Локальный доступ: http://localhost:%d\n", server->port);
    printf("Сервер готов к работе!\n\n");
    
    // Основной цикл сервера
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server->socket_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (server->running) {
                perror("Ошибка принятия соединения");
            }
            continue;
        }
        
        // Создаем новый поток для обработки соединения
        connection_data_t *conn_data = malloc(sizeof(connection_data_t));
        if (conn_data) {
            conn_data->client_fd = client_fd;
            conn_data->server = server;
            
            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_connection, conn_data) == 0) {
                pthread_detach(thread);
            } else {
                close(client_fd);
                free(conn_data);
            }
        } else {
            close(client_fd);
        }
    }
    
    return 0;
}

/**
 * Остановка HTTP сервера
 */
void http_server_stop(http_server_t *server) {
    if (server) {
        server->running = 0;
    }
}

/**
 * Очистка ресурсов сервера
 */
void http_server_cleanup(http_server_t *server) {
    if (server && server->socket_fd >= 0) {
        close(server->socket_fd);
        server->socket_fd = -1;
    }
}