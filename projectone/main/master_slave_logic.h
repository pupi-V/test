#ifndef MASTER_SLAVE_LOGIC_H
#define MASTER_SLAVE_LOGIC_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Типы устройств в системе зарядных станций
 */
typedef enum {
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_MASTER,
    DEVICE_TYPE_SLAVE
} device_type_t;

/**
 * @brief Состояния устройства
 */
typedef enum {
    DEVICE_STATE_INIT = 0,
    DEVICE_STATE_RUNNING,
    DEVICE_STATE_STOPPED,
    DEVICE_STATE_ERROR
} device_state_t;

/**
 * @brief Инициализация WiFi в режиме станции
 * @return ESP_OK при успехе, ESP_FAIL при ошибке
 */
esp_err_t wifi_init_sta(void);

/**
 * @brief Инициализация master/slave логики
 * @param device_type Тип устройства (MASTER или SLAVE)
 * @return ESP_OK при успехе, ESP_FAIL при ошибке
 */
esp_err_t master_slave_init(device_type_t device_type);

/**
 * @brief Запуск master/slave задач
 * @return ESP_OK при успехе, ESP_FAIL при ошибке
 */
esp_err_t master_slave_start(void);

/**
 * @brief Остановка master/slave задач
 * @return ESP_OK при успехе, ESP_FAIL при ошибке
 */
esp_err_t master_slave_stop(void);

/**
 * @brief Получение текущего типа устройства
 * @return Тип устройства
 */
device_type_t get_device_type(void);

/**
 * @brief Получение текущего состояния устройства
 * @return Состояние устройства
 */
device_state_t get_device_state(void);

/**
 * @brief Главная функция запуска системы зарядных станций
 */
void master_slave_run(void);

#ifdef __cplusplus
}
#endif

#endif // MASTER_SLAVE_LOGIC_H