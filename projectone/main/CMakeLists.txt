idf_component_register(
    SRCS
        "main.c"
        "udp_comm.c"
        "master_slave_logic.c"
        "charging_station_handlers.c"
        "udp_comm.c"
    INCLUDE_DIRS "."
    REQUIRES
        mdns
        esp_wifi
        nvs_flash
        esp_http_server
        esp_http_client
        esp_https_server
        esp_https_ota
        esp_netif
        freertos
        lwip
        log
        json
        esp_eth
)

target_add_binary_data(${COMPONENT_TARGET} "page.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "style.css" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "script.js" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "favicon.ico" BINARY)
target_add_binary_data(${COMPONENT_TARGET} "config.json" TEXT)

# Новые файлы веб интерфейса зарядных станций
target_add_binary_data(${COMPONENT_TARGET} "charging-station.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "charging-station.css" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "charging-station.js" TEXT)
