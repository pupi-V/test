#ifndef UDP_COMM_H
#define UDP_COMM_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация UDP коммуникации
 * @return ESP_OK при успехе, ESP_FAIL при ошибке
 */
esp_err_t udp_comm_init(void);

/**
 * @brief Отправка UDP сообщения на указанный адрес
 * @param ip_address IP адрес получателя
 * @param port Порт получателя
 * @param message Сообщение для отправки
 * @return ESP_OK при успехе, ESP_FAIL при ошибке
 */
esp_err_t udp_send_message(const char* ip_address, uint16_t port, const char* message);

/**
 * @brief Получение UDP сообщения
 * @param buffer Буфер для сообщения
 * @param buffer_size Размер буфера
 * @param timeout_ms Таймаут в миллисекундах
 * @return ESP_OK при успехе, ESP_ERR_TIMEOUT при таймауте, ESP_FAIL при ошибке
 */
esp_err_t udp_receive_message(char* buffer, size_t buffer_size, uint32_t timeout_ms);

/**
 * @brief Отправка широковещательного сообщения
 * @param message Сообщение для отправки
 * @return ESP_OK при успехе, ESP_FAIL при ошибке
 */
esp_err_t udp_broadcast_message(const char* message);

/**
 * @brief Деинициализация UDP коммуникации
 */
void udp_comm_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // UDP_COMM_H