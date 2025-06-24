#!/bin/bash

# Скрипт для переключения между Node.js и C сервером
# Автор: Система управления зарядными станциями

set -e

# Цвета для вывода
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Функция для вывода цветного текста
print_colored() {
    echo -e "${1}${2}${NC}"
}

# Функция для вывода заголовка
print_header() {
    echo
    print_colored $BLUE "=========================================="
    print_colored $BLUE "$1"
    print_colored $BLUE "=========================================="
    echo
}

# Функция для проверки зависимостей C сервера
check_c_dependencies() {
    print_colored $YELLOW "Проверка зависимостей C сервера..."
    
    # Проверяем наличие необходимых библиотек
    missing_deps=()
    
    if ! pkg-config --exists libmicrohttpd; then
        missing_deps+=("libmicrohttpd-dev")
    fi
    
    if ! pkg-config --exists libcjson; then
        missing_deps+=("libcjson-dev")
    fi
    
    if ! pkg-config --exists libpq; then
        missing_deps+=("libpq-dev")
    fi
    
    if ! pkg-config --exists libcurl; then
        missing_deps+=("libcurl4-openssl-dev")
    fi
    
    if ! command -v gcc &> /dev/null; then
        missing_deps+=("build-essential")
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        print_colored $RED "Отсутствуют зависимости: ${missing_deps[*]}"
        print_colored $YELLOW "Установите их командой:"
        echo "sudo apt-get install ${missing_deps[*]}"
        echo "или выполните: make deps-ubuntu"
        return 1
    fi
    
    print_colored $GREEN "Все зависимости установлены ✓"
    return 0
}

# Функция для сборки C сервера
build_c_server() {
    print_colored $YELLOW "Сборка C сервера..."
    
    cd server_c
    
    if make clean && make debug; then
        print_colored $GREEN "C сервер успешно собран ✓"
        cd ..
        return 0
    else
        print_colored $RED "Ошибка сборки C сервера ✗"
        cd ..
        return 1
    fi
}

# Функция для остановки текущих серверов
stop_servers() {
    print_colored $YELLOW "Остановка текущих серверов..."
    
    # Останавливаем Node.js сервер
    pkill -f "tsx server/index.ts" 2>/dev/null || true
    pkill -f "node.*server" 2>/dev/null || true
    
    # Останавливаем C сервер
    pkill -f "charging_station_server" 2>/dev/null || true
    
    # Ждем завершения процессов
    sleep 2
    
    print_colored $GREEN "Серверы остановлены ✓"
}

# Функция для запуска Node.js сервера
start_nodejs_server() {
    print_header "Запуск Node.js сервера"
    
    # Проверяем наличие node_modules
    if [ ! -d "node_modules" ]; then
        print_colored $YELLOW "Установка Node.js зависимостей..."
        npm install
    fi
    
    print_colored $GREEN "Запуск Node.js сервера на порту ${PORT:-5000}..."
    exec npm run dev
}

# Функция для запуска C сервера
start_c_server() {
    print_header "Запуск C сервера"
    
    # Проверяем зависимости
    if ! check_c_dependencies; then
        exit 1
    fi
    
    # Собираем сервер
    if ! build_c_server; then
        exit 1
    fi
    
    # Запускаем C сервер
    print_colored $GREEN "Запуск C сервера на порту ${PORT:-5000}..."
    cd server_c
    exec ./charging_station_server
}

# Функция для показа статуса
show_status() {
    print_header "Статус серверов"
    
    # Проверяем Node.js сервер
    if pgrep -f "tsx server/index.ts" > /dev/null; then
        print_colored $GREEN "Node.js сервер: Запущен ✓"
    else
        print_colored $YELLOW "Node.js сервер: Остановлен"
    fi
    
    # Проверяем C сервер
    if pgrep -f "charging_station_server" > /dev/null; then
        print_colored $GREEN "C сервер: Запущен ✓"
    else
        print_colored $YELLOW "C сервер: Остановлен"
    fi
    
    # Проверяем доступность порта
    if curl -s http://localhost:${PORT:-5000}/api/stations > /dev/null 2>&1; then
        print_colored $GREEN "API доступен на порту ${PORT:-5000} ✓"
    else
        print_colored $YELLOW "API недоступен на порту ${PORT:-5000}"
    fi
    
    echo
}

# Функция для показа помощи
show_help() {
    print_header "Система управления зарядными станциями - Переключатель серверов"
    
    echo "Использование: $0 [КОМАНДА]"
    echo
    echo "Команды:"
    echo "  nodejs     Запустить Node.js сервер (Express + TypeScript)"
    echo "  c          Запустить C сервер (libmicrohttpd)"
    echo "  stop       Остановить все серверы"
    echo "  status     Показать статус серверов"
    echo "  build      Собрать C сервер без запуска"
    echo "  deps       Проверить зависимости C сервера"
    echo "  help       Показать эту справку"
    echo
    echo "Переменные окружения:"
    echo "  PORT       Порт сервера (по умолчанию: 5000)"
    echo "  HOST       Хост сервера (по умолчанию: 0.0.0.0)"
    echo
    echo "Примеры:"
    echo "  $0 nodejs                    # Запустить Node.js сервер"
    echo "  $0 c                         # Запустить C сервер"
    echo "  PORT=8080 $0 nodejs          # Запустить на порту 8080"
    echo "  $0 stop && $0 c              # Переключиться на C сервер"
    echo
}

# Основная логика
main() {
    case "${1:-help}" in
        "nodejs"|"node"|"js")
            stop_servers
            start_nodejs_server
            ;;
        "c"|"native")
            stop_servers
            start_c_server
            ;;
        "stop")
            stop_servers
            ;;
        "status")
            show_status
            ;;
        "build")
            print_header "Сборка C сервера"
            check_c_dependencies && build_c_server
            ;;
        "deps")
            check_c_dependencies
            ;;
        "help"|"--help"|"-h")
            show_help
            ;;
        *)
            print_colored $RED "Неизвестная команда: $1"
            echo
            show_help
            exit 1
            ;;
    esac
}

# Проверяем, что скрипт запущен из корня проекта
if [ ! -f "package.json" ] || [ ! -d "server" ]; then
    print_colored $RED "Ошибка: Скрипт должен быть запущен из корня проекта"
    exit 1
fi

# Запускаем основную функцию
main "$@"