/**
 * Основной файл ESP32 системы управления зарядными станциями
 * Инициализирует WiFi, HTTP сервер и логику master/slave
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// Локальные заголовки
#include "simple_wifi.h"
#include "master_slave_logic.h"
#include "charging_station_handlers.h"
#include "udp_comm.h"

static const char *TAG = "charging_station_main";

/**
 * Основная функция приложения
 * Инициализирует все компоненты системы
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Запуск системы управления зарядными станциями");
    
    // Инициализация NVS (Non-Volatile Storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "NVS инициализирован");
    
    // Инициализация WiFi и подключение к сети
    ESP_LOGI(TAG, "Инициализация WiFi...");
    esp_err_t wifi_ret = wifi_init_sta();
    if (wifi_ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка инициализации WiFi: %s", esp_err_to_name(wifi_ret));
        return;
    }
    
    ESP_LOGI(TAG, "WiFi подключен успешно");
    
    // Инициализация HTTP сервера с веб интерфейсом
    ESP_LOGI(TAG, "Запуск HTTP сервера...");
    esp_err_t server_ret = start_charging_station_server();
    if (server_ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка запуска HTTP сервера: %s", esp_err_to_name(server_ret));
        return;
    }
    
    ESP_LOGI(TAG, "HTTP сервер запущен успешно");
    
    // Инициализация UDP коммуникации для связи между платами
    ESP_LOGI(TAG, "Инициализация UDP коммуникации...");
    esp_err_t udp_ret = udp_comm_init();
    if (udp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка инициализации UDP: %s", esp_err_to_name(udp_ret));
        // UDP не критично - продолжаем работу
    } else {
        ESP_LOGI(TAG, "UDP коммуникация инициализирована");
    }
    
    // Определение роли устройства и запуск соответствующей логики
    ESP_LOGI(TAG, "Определение роли устройства...");
    device_role_t role = determine_device_role();
    
    if (role == DEVICE_ROLE_MASTER) {
        ESP_LOGI(TAG, "Устройство работает в режиме MASTER");
        start_master_logic();
    } else if (role == DEVICE_ROLE_SLAVE) {
        ESP_LOGI(TAG, "Устройство работает в режиме SLAVE");
        start_slave_logic();
    } else {
        ESP_LOGW(TAG, "Роль устройства не определена, работаем в автономном режиме");
    }
    
    ESP_LOGI(TAG, "Система управления зарядными станциями запущена");
    ESP_LOGI(TAG, "Веб интерфейс доступен по адресу: http://[IP_УСТРОЙСТВА]/charging-station");
    
    // Основной цикл - мониторинг системы
    while (1) {
        // Проверка состояния системы каждые 30 секунд
        vTaskDelay(30000 / portTICK_PERIOD_MS);
        
        // Простая диагностика
        size_t free_heap = esp_get_free_heap_size();
        ESP_LOGI(TAG, "Свободная память: %d байт", free_heap);
        
        if (free_heap < 10000) {
            ESP_LOGW(TAG, "Низкий уровень свободной памяти!");
        }
    }
}