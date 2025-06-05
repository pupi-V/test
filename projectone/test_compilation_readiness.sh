#!/bin/bash

# Скрипт проверки готовности ESP32 проекта к компиляции
# Проверяет все критические файлы и конфигурации

echo "=== Проверка готовности ESP32 проекта к компиляции ==="
echo

# Проверка наличия основных файлов
echo "1. Проверка основных файлов:"
files=(
    "CMakeLists.txt"
    "sdkconfig.defaults"
    "main/CMakeLists.txt"
    "main/main.c"
    "main/simple_wifi.c"
    "main/simple_wifi.h"
    "main/master_slave_logic.c"
    "main/master_slave_logic.h"
    "main/udp_comm.c"
    "main/udp_comm.h"
    "main/charging_station_handlers.c"
    "main/charging_station_handlers.h"
    "main/idf_component.yml"
    "main/charging-station.html"
    "main/charging-station.css"
    "main/charging-station.js"
    "main/config.json"
)

for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "   ✓ $file"
    else
        echo "   ✗ $file - ОТСУТСТВУЕТ"
    fi
done
echo

# Проверка зависимостей в idf_component.yml
echo "2. Проверка зависимостей:"
if grep -q "protocol_examples_common" main/idf_component.yml; then
    echo "   ✗ Найдена проблемная зависимость protocol_examples_common"
else
    echo "   ✓ protocol_examples_common удален"
fi

if grep -q "espressif/mdns" main/idf_component.yml; then
    echo "   ✓ mdns зависимость присутствует"
else
    echo "   ✗ mdns зависимость отсутствует"
fi
echo

# Проверка конфигурации bootloader
echo "3. Проверка конфигурации bootloader:"
if grep -q "CONFIG_BOOTLOADER_LOG_LEVEL_INFO" sdkconfig.defaults; then
    echo "   ✓ Настройки bootloader добавлены"
else
    echo "   ✗ Настройки bootloader отсутствуют"
fi

if grep -q "CONFIG_ESPTOOLPY_FLASHMODE_DIO" sdkconfig.defaults; then
    echo "   ✓ Настройки Flash добавлены"
else
    echo "   ✗ Настройки Flash отсутствуют"
fi
echo

# Проверка CMake версии
echo "4. Проверка CMakeLists.txt:"
if grep -q "cmake_minimum_required(VERSION 3.5)" CMakeLists.txt; then
    echo "   ✓ CMake версия совместима с ESP-IDF"
else
    echo "   ✗ CMake версия может быть несовместима"
fi

if grep -q "simple_wifi.c" main/CMakeLists.txt; then
    echo "   ✓ simple_wifi.c добавлен в сборку"
else
    echo "   ✗ simple_wifi.c не добавлен в сборку"
fi
echo

# Проверка файла dependencies.lock
echo "5. Проверка кеша зависимостей:"
if [ -f "dependencies.lock" ]; then
    echo "   ✗ dependencies.lock присутствует (может вызвать проблемы)"
    echo "     Рекомендуется удалить: rm dependencies.lock"
else
    echo "   ✓ dependencies.lock отсутствует"
fi
echo

# Проверка настроек WiFi
echo "6. Проверка настроек WiFi:"
if grep -q 'WIFI_SSID.*".*"' main/master_slave_logic.c; then
    ssid=$(grep 'WIFI_SSID' main/master_slave_logic.c | head -1)
    echo "   ✓ SSID настроен: $ssid"
else
    echo "   ⚠ SSID требует настройки в main/master_slave_logic.c"
fi

if grep -q 'WIFI_PASS.*".*"' main/master_slave_logic.c; then
    echo "   ✓ Пароль WiFi настроен"
else
    echo "   ⚠ Пароль WiFi требует настройки в main/master_slave_logic.c"
fi
echo

echo "=== Рекомендуемые команды для компиляции ==="
echo "cd projectone"
echo "idf.py fullclean"
echo "idf.py build"
echo "idf.py -p /dev/ttyUSB0 flash monitor"
echo

echo "=== Ожидаемый результат ==="
echo "После успешной загрузки ESP32 должен:"
echo "- Подключиться к WiFi сети"
echo "- Запустить HTTP сервер на порту 80"
echo "- Предоставить веб интерфейс по адресу http://[IP]/charging-station"
echo "- Обеспечить UDP коммуникацию между платами"
echo
echo "Проверка завершена."