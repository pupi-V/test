/**
 * Реализация клиента для работы с ESP32 платами
 * Поддерживает сканирование сети, подключение и обмен данными
 */

#include "esp32_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <time.h>
#include <cjson/cJSON.h>

/**
 * Callback функция для обработки ответа CURL
 */
size_t curl_write_callback(void *contents, size_t size, size_t nmemb, http_response_t *response) {
    size_t total_size = size * nmemb;
    
    char *new_data = realloc(response->data, response->size + total_size + 1);
    if (!new_data) {
        printf("Ошибка выделения памяти для HTTP ответа\n");
        return 0;
    }
    
    response->data = new_data;
    memcpy(&(response->data[response->size]), contents, total_size);
    response->size += total_size;
    response->data[response->size] = '\0';
    
    return total_size;
}

/**
 * Освобождение памяти HTTP ответа
 */
void free_http_response(http_response_t *response) {
    if (response && response->data) {
        free(response->data);
        response->data = NULL;
        response->size = 0;
    }
}

/**
 * Получение информации о локальной сети
 */
int get_local_network_info(char *network_base, char *subnet_mask) {
    struct ifaddrs *ifaddrs_ptr = NULL;
    
    if (getifaddrs(&ifaddrs_ptr) == -1) {
        perror("getifaddrs");
        return -1;
    }
    
    for (struct ifaddrs *ifa = ifaddrs_ptr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        // Ищем IPv4 интерфейсы (исключая loopback)
        if (ifa->ifa_addr->sa_family == AF_INET && 
            strcmp(ifa->ifa_name, "lo") != 0) {
            
            struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
            struct sockaddr_in *mask_in = (struct sockaddr_in *)ifa->ifa_netmask;
            
            // Получаем IP адрес и маску подсети
            char ip_str[INET_ADDRSTRLEN];
            char mask_str[INET_ADDRSTRLEN];
            
            inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &mask_in->sin_addr, mask_str, INET_ADDRSTRLEN);
            
            // Вычисляем базовый адрес сети
            uint32_t ip_addr = ntohl(addr_in->sin_addr.s_addr);
            uint32_t mask_addr = ntohl(mask_in->sin_addr.s_addr);
            uint32_t network_addr = ip_addr & mask_addr;
            
            struct in_addr network_in_addr;
            network_in_addr.s_addr = htonl(network_addr);
            inet_ntop(AF_INET, &network_in_addr, network_base, INET_ADDRSTRLEN);
            strcpy(subnet_mask, mask_str);
            
            printf("Найдена локальная сеть: %s/%s (интерфейс: %s)\n", 
                   network_base, mask_str, ifa->ifa_name);
            
            freeifaddrs(ifaddrs_ptr);
            return 0;
        }
    }
    
    freeifaddrs(ifaddrs_ptr);
    printf("Не удалось определить локальную сеть\n");
    return -1;
}

/**
 * Проверка доступности хоста через ping (упрощенная версия)
 */
int esp32_ping_board(const char *ip) {
    char ping_cmd[256];
    snprintf(ping_cmd, sizeof(ping_cmd), "ping -c 1 -W 1 %s > /dev/null 2>&1", ip);
    
    int result = system(ping_cmd);
    return (result == 0) ? 1 : 0;
}

/**
 * Проверка является ли устройство ESP32 зарядной станцией
 */
int esp32_check_charging_board(const char *ip, esp32_board_info_t *board_info) {
    CURL *curl;
    CURLcode res;
    http_response_t response = {0};
    
    curl = curl_easy_init();
    if (!curl) {
        return -1;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s/api/info", ip);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        free_http_response(&response);
        return -1;
    }
    
    // Парсим JSON ответ
    if (parse_esp32_response(response.data, board_info) == 0) {
        strcpy(board_info->ip, ip);
        strcpy(board_info->status, "online");
        
        // Получаем текущее время для last_seen
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(board_info->last_seen, sizeof(board_info->last_seen), 
                "%Y-%m-%d %H:%M:%S", tm_info);
        
        free_http_response(&response);
        return 0;
    }
    
    free_http_response(&response);
    return -1;
}

/**
 * Парсинг JSON ответа от ESP32
 */
int parse_esp32_response(const char *json_data, esp32_board_info_t *board_info) {
    if (!json_data || !board_info) {
        return -1;
    }
    
    cJSON *json = cJSON_Parse(json_data);
    if (!json) {
        return -1;
    }
    
    // Очищаем структуру
    memset(board_info, 0, sizeof(esp32_board_info_t));
    
    // Парсим основные поля
    cJSON *item = cJSON_GetObjectItem(json, "id");
    if (cJSON_IsString(item)) {
        strncpy(board_info->id, item->valuestring, ESP32_MAX_STRING - 1);
    }
    
    item = cJSON_GetObjectItem(json, "type");
    if (cJSON_IsString(item)) {
        strncpy(board_info->type, item->valuestring, 31);
    } else {
        strcpy(board_info->type, "slave"); // По умолчанию slave
    }
    
    item = cJSON_GetObjectItem(json, "name");
    if (cJSON_IsString(item)) {
        strncpy(board_info->name, item->valuestring, ESP32_MAX_STRING - 1);
    } else {
        strcpy(board_info->name, "ESP32 Station");
    }
    
    item = cJSON_GetObjectItem(json, "technicalName");
    if (cJSON_IsString(item)) {
        strncpy(board_info->technical_name, item->valuestring, ESP32_MAX_STRING - 1);
    } else {
        strcpy(board_info->technical_name, "ESP32-001");
    }
    
    item = cJSON_GetObjectItem(json, "maxPower");
    if (cJSON_IsNumber(item)) {
        board_info->max_power = (float)item->valuedouble;
    } else {
        board_info->max_power = 22.0; // Значение по умолчанию
    }
    
    cJSON_Delete(json);
    return 0;
}

/**
 * Сканирование одного IP адреса
 */
static int scan_single_ip(const char *ip, esp32_board_info_t *board_info) {
    // Сначала проверяем доступность хоста
    if (!esp32_ping_board(ip)) {
        return -1; // Хост недоступен
    }
    
    // Проверяем, является ли это ESP32 зарядной станцией
    return esp32_check_charging_board(ip, board_info);
}

/**
 * Сканирование локальной сети для поиска ESP32 плат
 */
int esp32_scan_network(esp32_board_info_t **boards, int *count) {
    char network_base[INET_ADDRSTRLEN];
    char subnet_mask[INET_ADDRSTRLEN];
    
    *boards = NULL;
    *count = 0;
    
    // Получаем информацию о локальной сети
    if (get_local_network_info(network_base, subnet_mask) != 0) {
        return -1;
    }
    
    // Парсим базовый адрес сети
    struct in_addr network_addr;
    if (inet_pton(AF_INET, network_base, &network_addr) != 1) {
        printf("Ошибка парсинга сетевого адреса\n");
        return -1;
    }
    
    uint32_t base_ip = ntohl(network_addr.s_addr);
    
    // Для простоты сканируем /24 подсеть (256 адресов)
    // В реальном проекте можно реализовать более сложную логику по маске
    const int max_boards = 256;
    esp32_board_info_t *found_boards = malloc(max_boards * sizeof(esp32_board_info_t));
    if (!found_boards) {
        return -1;
    }
    
    int found_count = 0;
    
    // Сканируем адреса с .1 по .254
    for (int i = 1; i < 255 && found_count < max_boards; i++) {
        uint32_t scan_ip = (base_ip & 0xFFFFFF00) | i;
        
        struct in_addr addr;
        addr.s_addr = htonl(scan_ip);
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);
        
        esp32_board_info_t board_info;
        if (scan_single_ip(ip_str, &board_info) == 0) {
            memcpy(&found_boards[found_count], &board_info, sizeof(esp32_board_info_t));
            found_count++;
            printf("Найдена ESP32 плата: %s (%s) на %s\n", 
                   board_info.name, board_info.type, ip_str);
        }
    }
    
    printf("Сканирование завершено. Найдено ESP32 плат: %d\n", found_count);
    
    if (found_count > 0) {
        // Сжимаем массив до реального размера
        *boards = realloc(found_boards, found_count * sizeof(esp32_board_info_t));
        *count = found_count;
    } else {
        free(found_boards);
        *boards = NULL;
        *count = 0;
    }
    
    return 0;
}

/**
 * Подключение к конкретной ESP32 плате по IP
 */
int esp32_connect_to_board(const char *ip, const char *expected_type, esp32_board_info_t *board_info) {
    if (!ip || !board_info) {
        return -1;
    }
    
    printf("Попытка подключения к ESP32 плате %s%s%s...\n", 
           ip, expected_type ? " (тип: " : "", expected_type ? expected_type : "");
    printf(expected_type ? ")\n" : "");
    
    // Проверяем доступность и получаем информацию о плате
    if (esp32_check_charging_board(ip, board_info) != 0) {
        printf("Плата по адресу %s не отвечает или недоступна\n", ip);
        return -1;
    }
    
    // Проверяем соответствие типа, если указан
    if (expected_type && strcmp(board_info->type, expected_type) != 0) {
        printf("Тип платы (%s) не соответствует ожидаемому (%s)\n", 
               board_info->type, expected_type);
        return -1;
    }
    
    printf("Плата найдена: %s (%s)\n", board_info->name, board_info->type);
    return 0;
}

/**
 * Отправка данных на ESP32 плату
 */
int esp32_send_data(const char *ip, const char *json_data) {
    if (!ip || !json_data) {
        return -1;
    }
    
    CURL *curl;
    CURLcode res;
    http_response_t response = {0};
    
    curl = curl_easy_init();
    if (!curl) {
        return -1;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s/api/station", ip);
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    
    res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free_http_response(&response);
    
    if (res != CURLE_OK) {
        printf("Ошибка отправки данных на ESP32 %s: %s\n", ip, curl_easy_strerror(res));
        return -1;
    }
    
    return 0;
}

/**
 * Получение данных с ESP32 платы
 */
int esp32_get_data(const char *ip, char **response_data) {
    if (!ip || !response_data) {
        return -1;
    }
    
    CURL *curl;
    CURLcode res;
    http_response_t response = {0};
    
    curl = curl_easy_init();
    if (!curl) {
        return -1;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "http://%s/api/station", ip);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        printf("Ошибка получения данных с ESP32 %s: %s\n", ip, curl_easy_strerror(res));
        free_http_response(&response);
        return -1;
    }
    
    *response_data = response.data;
    return 0;
}