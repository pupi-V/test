
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"

// Объявляем функцию запуска React сервера
extern httpd_handle_t start_react_webserver(void);

static const char *TAG = "ESP32_REACT";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 with React interface");

    // Инициализация NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Инициализация WiFi (ваш код WiFi подключения)
    // ... WiFi setup код ...
    
    // Задержка для подключения к WiFi
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
    // Запуск React веб-сервера
    httpd_handle_t server = start_react_webserver();
    
    if (server) {
        ESP_LOGI(TAG, "React interface ready!");
        ESP_LOGI(TAG, "Open http://[ESP32_IP]/ in your browser");
    } else {
        ESP_LOGE(TAG, "Failed to start React interface");
    }
}
