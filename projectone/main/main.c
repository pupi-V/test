/* Simple HTTP + SSL Server Example

This example code is in the Public Domain (or CC0 licensed, at your option.)

Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"
#include <esp_https_server.h>
#include "esp_tls.h"
#include "sdkconfig.h"
#include <esp_http_server.h>
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "esp_event.h"
#include "esp_wifi.h"
#include "mdns.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "master_slave_logic.h"





/* A simple example that demonstrates how to create GET and POST
* handlers and start an HTTPS server.
*/
void assign_slave_ranks(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data);

static const char *TAG = "example";
extern const char _binary_page_html_start[]; // Начало файла
extern const char _binary_page_html_end[];   // Конец файла
extern const char _binary_style_css_start[];
extern const char _binary_style_css_end[];
extern const char _binary_script_js_start[];
extern const char _binary_script_js_end[];
extern const uint8_t _binary_favicon_ico_start[];
extern const uint8_t _binary_favicon_ico_end[];


/* Event handler for catching system events */
static void event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
    {
        if (event_base == ESP_HTTPS_SERVER_EVENT) {
            if (event_id == HTTPS_SERVER_EVENT_ERROR) {
                esp_https_server_last_error_t *last_error = (esp_tls_last_error_t *) event_data;
            ESP_LOGE(TAG, "Error event triggered: last_error = %s, last_tls_err = %d, tls_flag = %d", esp_err_to_name(last_error->last_error), last_error->esp_tls_error_code, last_error->esp_tls_flags);
        }
    }
}

/* An HTTP GET handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    size_t html_size = _binary_page_html_end - _binary_page_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, _binary_page_html_start, html_size);
    return ESP_OK;
}

static esp_err_t style_css_handler(httpd_req_t *req)
{
    size_t css_size = _binary_style_css_end - _binary_style_css_start;
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, _binary_style_css_start, css_size);
    return ESP_OK;
}

static esp_err_t script_js_handler(httpd_req_t *req)
{
    size_t js_size = _binary_script_js_end - _binary_script_js_start;
    if (js_size > 0) js_size -= 1; // Уменьшаем размер на 1 байт
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, _binary_script_js_start, js_size);
    return ESP_OK;
}

static esp_err_t favicon_handler(httpd_req_t *req)
{
    size_t icon_size = _binary_favicon_ico_end - _binary_favicon_ico_start;
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)_binary_favicon_ico_start, icon_size);
    return ESP_OK;
}

#if CONFIG_EXAMPLE_ENABLE_HTTPS_USER_CALLBACK
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
static void print_peer_cert_info(const mbedtls_ssl_context *ssl)
{
    const mbedtls_x509_crt *cert;
    const size_t buf_size = 1024;
    char *buf = calloc(buf_size, sizeof(char));
    if (buf == NULL) {
        ESP_LOGE(TAG, "Out of memory - Callback execution failed!");
        return;
    }

    // Logging the peer certificate info
    cert = mbedtls_ssl_get_peer_cert(ssl);
    if (cert != NULL) {
        mbedtls_x509_crt_info((char *) buf, buf_size - 1, "    ", cert);
        ESP_LOGI(TAG, "Peer certificate info:\n%s", buf);
    } else {
        ESP_LOGW(TAG, "Could not obtain the peer certificate!");
    }

    free(buf);
}
#endif
/**
 * Example callback function to get the certificate of connected clients,
 * whenever a new SSL connection is created and closed
 *
 * Can also be used to other information like Socket FD, Connection state, etc.
 *
 * NOTE: This callback will not be able to obtain the client certificate if the
 * following config `Set minimum Certificate Verification mode to Optional` is
 * not enabled (enabled by default in this example).
 *
 * The config option is found here - Component config → ESP-TLS
 *
 */
static void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb)
{
    ESP_LOGI(TAG, "User callback invoked!");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
    mbedtls_ssl_context *ssl_ctx = NULL;
#endif
    switch(user_cb->user_cb_state) {
        case HTTPD_SSL_USER_CB_SESS_CREATE:
            ESP_LOGD(TAG, "At session creation");

            // Logging the socket FD
            int sockfd = -1;
            esp_err_t esp_ret;
            esp_ret = esp_tls_get_conn_sockfd(user_cb->tls, &sockfd);
            if (esp_ret != ESP_OK) {
                ESP_LOGE(TAG, "Error in obtaining the sockfd from tls context");
                break;
            }
            ESP_LOGI(TAG, "Socket FD: %d", sockfd);
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            // Logging the current ciphersuite
            ESP_LOGI(TAG, "Current Ciphersuite: %s", mbedtls_ssl_get_ciphersuite(ssl_ctx));
#endif
            break;

        case HTTPD_SSL_USER_CB_SESS_CLOSE:
            ESP_LOGD(TAG, "At session close");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            // Logging the peer certificate
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            print_peer_cert_info(ssl_ctx);
#endif
            break;
        default:
            ESP_LOGE(TAG, "Illegal state!");
            return;
    }
}
#endif

// Прототипы обработчиков POST-запросов
static esp_err_t select1_post_handler(httpd_req_t *req);
static esp_err_t select2_post_handler(httpd_req_t *req);
static esp_err_t select3_post_handler(httpd_req_t *req);

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

static const httpd_uri_t style_css = {
    .uri       = "/style.css",
    .method    = HTTP_GET,
    .handler   = style_css_handler
};

static const httpd_uri_t script_js = {
    .uri       = "/script.js",
    .method    = HTTP_GET,
    .handler   = script_js_handler
};

static const httpd_uri_t select1 = {
    .uri       = "/select1",
    .method    = HTTP_POST,
    .handler   = select1_post_handler
};

static const httpd_uri_t select2 = {
    .uri       = "/select2",
    .method    = HTTP_POST,
    .handler   = select2_post_handler
};

static const httpd_uri_t select3 = {
    .uri       = "/select3",
    .method    = HTTP_POST,
    .handler   = select3_post_handler
};

static const httpd_uri_t favicon = {
    .uri       = "/favicon.ico",
    .method    = HTTP_GET,
    .handler   = favicon_handler
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting HTTP server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &style_css);
        httpd_register_uri_handler(server, &script_js);
        httpd_register_uri_handler(server, &select1);
        httpd_register_uri_handler(server, &select2);
        httpd_register_uri_handler(server, &select3);
        httpd_register_uri_handler(server, &favicon);
        // и для select2, select3
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_ssl_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Failed to stop https server");
        }
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_webserver();
    }
}

#define EXAMPLE_ESP_WIFI_SSID      "esp32_ap"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_MAX_STA_CONN       4





uint32_t start_send = 0;


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    master_slave_run();


}

/* An HTTP POST handler */
static esp_err_t select1_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)-1));
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = 0;


    char *selected = strstr(buf, "dropdown1=");
    if (selected) {
        selected += strlen("dropdown1=");
        ESP_LOGI(TAG, "Выбрано в первом списке: %s", selected);
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t select2_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)-1));
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = 0;

    char *selected = strstr(buf, "dropdown2=");
    if (selected) {
        selected += strlen("dropdown2=");
        ESP_LOGI(TAG, "Выбрано во втором списке: %s", selected);
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t select3_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)-1));
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = 0;

    char *selected = strstr(buf, "dropdown3=");
    if (selected) {
        selected += strlen("dropdown3=");
        ESP_LOGI(TAG, "Выбрано в третьем списке: %s", selected);
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

