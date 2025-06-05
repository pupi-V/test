#ifndef CHARGING_STATION_HANDLERS_H
#define CHARGING_STATION_HANDLERS_H

#include <esp_http_server.h>

// Прототипы обработчиков веб интерфейса
esp_err_t charging_station_get_handler(httpd_req_t *req);
esp_err_t charging_station_css_handler(httpd_req_t *req);
esp_err_t charging_station_js_handler(httpd_req_t *req);

// Прототипы API обработчиков
esp_err_t api_stations_get_handler(httpd_req_t *req);
esp_err_t api_station_update_handler(httpd_req_t *req);
esp_err_t api_esp32_scan_handler(httpd_req_t *req);
esp_err_t api_esp32_connect_handler(httpd_req_t *req);

#endif // CHARGING_STATION_HANDLERS_H