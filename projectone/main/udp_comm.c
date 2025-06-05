#include "udp_comm.h"
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

static const char *TAG = "UDP_COMM";

#define UDP_PORT 3333
#define MAX_MESSAGE_SIZE 1024

static int udp_socket = -1;
static bool udp_initialized = false;

esp_err_t udp_comm_init(void)
{
    if (udp_initialized) {
        return ESP_OK;
    }

    // Создаем UDP сокет
    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_socket < 0) {
        ESP_LOGE(TAG, "Не удалось создать UDP сокет: errno %d", errno);
        return ESP_FAIL;
    }

    // Настраиваем адрес для привязки
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    // Привязываем сокет к порту
    int err = bind(udp_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Не удалось привязать сокет: errno %d", errno);
        close(udp_socket);
        return ESP_FAIL;
    }

    udp_initialized = true;
    ESP_LOGI(TAG, "UDP коммуникация инициализирована на порту %d", UDP_PORT);
    
    return ESP_OK;
}

esp_err_t udp_send_message(const char* ip_address, uint16_t port, const char* message)
{
    if (!udp_initialized) {
        ESP_LOGE(TAG, "UDP не инициализирован");
        return ESP_FAIL;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(ip_address);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    int err = sendto(udp_socket, message, strlen(message), 0, 
                     (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    
    if (err < 0) {
        ESP_LOGE(TAG, "Ошибка отправки UDP сообщения: errno %d", errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "UDP сообщение отправлено на %s:%d: %s", ip_address, port, message);
    return ESP_OK;
}

esp_err_t udp_receive_message(char* buffer, size_t buffer_size, uint32_t timeout_ms)
{
    if (!udp_initialized) {
        ESP_LOGE(TAG, "UDP не инициализирован");
        return ESP_FAIL;
    }

    // Настраиваем таймаут
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_storage source_addr;
    socklen_t socklen = sizeof(source_addr);
    
    int len = recvfrom(udp_socket, buffer, buffer_size - 1, 0, 
                       (struct sockaddr *)&source_addr, &socklen);
    
    if (len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return ESP_ERR_TIMEOUT;
        }
        ESP_LOGE(TAG, "Ошибка получения UDP сообщения: errno %d", errno);
        return ESP_FAIL;
    }

    buffer[len] = 0; // Null-terminate
    
    // Получаем IP адрес отправителя
    char addr_str[128];
    if (source_addr.ss_family == AF_INET) {
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
    }
    
    ESP_LOGI(TAG, "UDP сообщение получено от %s: %s", addr_str, buffer);
    return ESP_OK;
}

esp_err_t udp_broadcast_message(const char* message)
{
    if (!udp_initialized) {
        ESP_LOGE(TAG, "UDP не инициализирован");
        return ESP_FAIL;
    }

    // Создаем отдельный сокет для широковещания
    int broadcast_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (broadcast_socket < 0) {
        ESP_LOGE(TAG, "Не удалось создать broadcast сокет: errno %d", errno);
        return ESP_FAIL;
    }

    // Включаем broadcast
    int broadcast_enable = 1;
    setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST, 
               &broadcast_enable, sizeof(broadcast_enable));

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    int err = sendto(broadcast_socket, message, strlen(message), 0, 
                     (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    
    close(broadcast_socket);
    
    if (err < 0) {
        ESP_LOGE(TAG, "Ошибка отправки broadcast сообщения: errno %d", errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Broadcast сообщение отправлено: %s", message);
    return ESP_OK;
}

void udp_comm_deinit(void)
{
    if (udp_socket >= 0) {
        close(udp_socket);
        udp_socket = -1;
    }
    udp_initialized = false;
    ESP_LOGI(TAG, "UDP коммуникация деинициализирована");
}