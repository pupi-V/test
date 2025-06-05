#ifndef SIMPLE_WIFI_H
#define SIMPLE_WIFI_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi in station mode and connect to AP
 * @param ssid WiFi SSID to connect to
 * @param password WiFi password
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t simple_wifi_init_sta(const char* ssid, const char* password);

#ifdef __cplusplus
}
#endif

#endif // SIMPLE_WIFI_H