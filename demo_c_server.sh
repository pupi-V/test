#!/bin/bash

# Демонстрационный скрипт для запуска C-сервера
# Показывает возможности переписанного бэкенда

echo "=================================================="
echo "Демонстрация C-сервера зарядных станций"
echo "=================================================="
echo

# Проверяем, собран ли C-сервер
if [ ! -f "server_c/charging_station_server" ]; then
    echo "Сборка C-сервера..."
    cd server_c
    make clean && make debug
    cd ..
fi

echo "🚀 Запуск C-сервера на порту 5001..."
echo "📍 API будет доступен по адресу: http://localhost:5001"
echo "🔗 Тестовые endpoints:"
echo "   GET http://localhost:5001/api/stations"
echo "   POST http://localhost:5001/api/esp32/scan"
echo

# Запускаем C-сервер на порту 5001 чтобы не конфликтовать с Node.js
PORT=5001 ./server_c/charging_station_server