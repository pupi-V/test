#include "master_slave_logic.h"
#include "udp_comm.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <cJSON.h>
#include <string.h>
#include <esp_http_server.h>
#include "charging_station_handlers.h"


static const char *TAG = "MASTER_SLAVE";

static device_type_t current_device_type = DEVICE_TYPE_UNKNOWN;
static device_state_t current_device_state = DEVICE_STATE_INIT;
static TaskHandle_t master_task_handle = NULL;
static TaskHandle_t slave_task_handle = NULL;
static httpd_handle_t server = NULL;

// WiFi event group
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// WiFi конфигурация
#define WIFI_SSID      "ESP32_Network"
#define WIFI_PASS      "esp32password"
#define WIFI_MAXIMUM_RETRY  5

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Повторное подключение к AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"Подключение к AP не удалось");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Получен IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi инициализация завершена");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Подключено к AP SSID:%s", WIFI_SSID);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Не удалось подключиться к SSID:%s", WIFI_SSID);
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "Неожиданное событие");
        return ESP_FAIL;
    }
}

static esp_err_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Запуск HTTP сервера на порту: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        
        // Регистрируем обработчики веб интерфейса зарядных станций
        httpd_uri_t charging_station_uri = {
            .uri       = "/charging-station",
            .method    = HTTP_GET,
            .handler   = charging_station_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &charging_station_uri);

        httpd_uri_t charging_station_css_uri = {
            .uri       = "/charging-station.css",
            .method    = HTTP_GET,
            .handler   = charging_station_css_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &charging_station_css_uri);

        httpd_uri_t charging_station_js_uri = {
            .uri       = "/charging-station.js",
            .method    = HTTP_GET,
            .handler   = charging_station_js_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &charging_station_js_uri);

        // API обработчики
        httpd_uri_t api_stations_uri = {
            .uri       = "/api/stations",
            .method    = HTTP_GET,
            .handler   = api_stations_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_stations_uri);

        httpd_uri_t api_station_update_uri = {
            .uri       = "/api/stations/*",
            .method    = HTTP_POST,
            .handler   = api_station_update_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_station_update_uri);

        httpd_uri_t api_esp32_scan_uri = {
            .uri       = "/api/esp32/scan",
            .method    = HTTP_POST,
            .handler   = api_esp32_scan_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_esp32_scan_uri);

        httpd_uri_t api_esp32_connect_uri = {
            .uri       = "/api/esp32/connect",
            .method    = HTTP_POST,
            .handler   = api_esp32_connect_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_esp32_connect_uri);

        return ESP_OK;
    }

    ESP_LOGI(TAG, "Ошибка запуска сервера!");
    return ESP_FAIL;
}

static void master_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Master задача запущена");
    current_device_state = DEVICE_STATE_RUNNING;

    char message_buffer[256];
    
    while (1) {
        // Отправляем статус зарядной станции
        cJSON *status = cJSON_CreateObject();
        cJSON_AddStringToObject(status, "type", "master");
        cJSON_AddStringToObject(status, "status", "online");
        cJSON_AddNumberToObject(status, "power", 22.0);
        cJSON_AddStringToObject(status, "id", "ESP32_MASTER_001");
        
        char *status_string = cJSON_Print(status);
        udp_broadcast_message(status_string);
        
        cJSON_Delete(status);
        free(status_string);

        // Ожидаем сообщения от slave устройств
        esp_err_t result = udp_receive_message(message_buffer, sizeof(message_buffer), 1000);
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Master получил сообщение: %s", message_buffer);
            
            // Парсим JSON сообщение
            cJSON *json = cJSON_Parse(message_buffer);
            if (json != NULL) {
                cJSON *type = cJSON_GetObjectItem(json, "type");
                if (cJSON_IsString(type) && strcmp(cJSON_GetStringValue(type), "slave") == 0) {
                    ESP_LOGI(TAG, "Обнаружено slave устройство");
                }
                cJSON_Delete(json);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Обновление каждые 5 секунд
    }
}

static void slave_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Slave задача запущена");
    current_device_state = DEVICE_STATE_RUNNING;

    char message_buffer[256];
    
    while (1) {
        // Отправляем статус slave устройства
        cJSON *status = cJSON_CreateObject();
        cJSON_AddStringToObject(status, "type", "slave");
        cJSON_AddStringToObject(status, "status", "online");
        cJSON_AddNumberToObject(status, "power", 11.0);
        cJSON_AddStringToObject(status, "id", "ESP32_SLAVE_001");
        
        char *status_string = cJSON_Print(status);
        udp_broadcast_message(status_string);
        
        cJSON_Delete(status);
        free(status_string);

        // Слушаем команды от master
        esp_err_t result = udp_receive_message(message_buffer, sizeof(message_buffer), 1000);
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Slave получил сообщение: %s", message_buffer);
            
            // Парсим команды от master
            cJSON *json = cJSON_Parse(message_buffer);
            if (json != NULL) {
                cJSON *type = cJSON_GetObjectItem(json, "type");
                if (cJSON_IsString(type) && strcmp(cJSON_GetStringValue(type), "master") == 0) {
                    ESP_LOGI(TAG, "Получена команда от master устройства");
                }
                cJSON_Delete(json);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(3000)); // Обновление каждые 3 секунды
    }
}

esp_err_t master_slave_init(device_type_t device_type)
{
    ESP_LOGI(TAG, "Инициализация устройства как %s", 
             device_type == DEVICE_TYPE_MASTER ? "MASTER" : "SLAVE");
    
    current_device_type = device_type;
    current_device_state = DEVICE_STATE_INIT;

    // Инициализация UDP коммуникации
    esp_err_t ret = udp_comm_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка инициализации UDP");
        return ret;
    }

    // Запуск веб сервера
    ret = start_webserver();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка запуска веб сервера");
        return ret;
    }

    return ESP_OK;
}

esp_err_t master_slave_start(void)
{
    if (current_device_type == DEVICE_TYPE_MASTER) {
        xTaskCreate(master_task, "master_task", 4096, NULL, 5, &master_task_handle);
    } else if (current_device_type == DEVICE_TYPE_SLAVE) {
        xTaskCreate(slave_task, "slave_task", 4096, NULL, 5, &slave_task_handle);
    } else {
        ESP_LOGE(TAG, "Неизвестный тип устройства");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t master_slave_stop(void)
{
    if (master_task_handle != NULL) {
        vTaskDelete(master_task_handle);
        master_task_handle = NULL;
    }
    
    if (slave_task_handle != NULL) {
        vTaskDelete(slave_task_handle);
        slave_task_handle = NULL;
    }

    if (server != NULL) {
        httpd_stop(server);
        server = NULL;
    }

    udp_comm_deinit();
    current_device_state = DEVICE_STATE_STOPPED;
    
    return ESP_OK;
}

device_type_t get_device_type(void)
{
    return current_device_type;
}

device_state_t get_device_state(void)
{
    return current_device_state;
}

void master_slave_run(void)
{
    // Определяем тип устройства (можно настроить через конфигурацию)
    device_type_t device_type = DEVICE_TYPE_MASTER; // По умолчанию master
    
    ESP_LOGI(TAG, "Запуск системы зарядных станций");
    
    // Инициализация WiFi
    esp_err_t ret = wifi_init_sta();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка инициализации WiFi");
        return;
    }

    // Инициализация master/slave логики
    ret = master_slave_init(device_type);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка инициализации master/slave");
        return;
    }

    // Запуск основной логики
    ret = master_slave_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка запуска master/slave");
        return;
    }

    ESP_LOGI(TAG, "Система зарядных станций запущена успешно");
}