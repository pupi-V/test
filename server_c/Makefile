# Makefile для сборки C-сервера системы управления зарядными станциями
# Упрощенная версия для совместимости с Nix окружением

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
INCLUDES = -I.
LIBS = -lpthread

# Имя исполняемого файла
TARGET = charging_station_server

# Исходные файлы
SOURCES = main.c storage_simple.c simple_http.c simple_json.c

# Объектные файлы
OBJECTS = $(SOURCES:.c=.o)

# Режимы сборки
DEBUG_CFLAGS = -g -O0 -DDEBUG
RELEASE_CFLAGS = -O2 -DNDEBUG

# Цель по умолчанию
all: debug

# Режим отладки
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: $(TARGET)

# Режим продакшена
release: CFLAGS += $(RELEASE_CFLAGS)
release: $(TARGET)

# Основная цель сборки
$(TARGET): $(OBJECTS)
	@echo "🔗 Линковка исполняемого файла: $(TARGET)"
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)
	@echo "✅ Сборка завершена: $(TARGET)"

# Компиляция объектных файлов
%.o: %.c
	@echo "🔨 Компиляция: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Очистка собранных файлов
clean:
	@echo "🧹 Очистка объектных файлов и исполняемого файла"
	rm -f $(OBJECTS) $(TARGET)

# Полная очистка включая временные файлы
distclean: clean
	@echo "🧹 Полная очистка"
	rm -f *~ *.bak core

# Установка зависимостей (для Ubuntu/Debian)
deps-ubuntu:
	@echo "📦 Установка зависимостей для Ubuntu/Debian"
	sudo apt-get update
	sudo apt-get install -y build-essential

# Установка зависимостей (для CentOS/RHEL)
deps-centos:
	@echo "📦 Установка зависимостей для CentOS/RHEL"
	sudo yum groupinstall -y "Development Tools"

# Запуск сервера в режиме разработки
run: debug
	@echo "🚀 Запуск сервера в режиме разработки"
	./$(TARGET)

# Запуск сервера с кастомным портом
run-port: debug
	@echo "🚀 Запуск сервера на порту $(PORT)"
	PORT=$(PORT) ./$(TARGET)

# Проверка синтаксиса
check:
	@echo "🔍 Проверка синтаксиса исходных файлов"
	$(CC) $(CFLAGS) $(INCLUDES) -fsyntax-only $(SOURCES)

# Создание архива с исходным кодом
archive:
	@echo "📦 Создание архива исходного кода"
	tar -czf charging_station_server_$(shell date +%Y%m%d_%H%M%S).tar.gz *.c *.h Makefile README.md

# Форматирование кода (требует clang-format)
format:
	@echo "🎨 Форматирование исходного кода"
	clang-format -i -style="{BasedOnStyle: GNU, IndentWidth: 4, TabWidth: 4}" *.c *.h

# Статический анализ кода (требует cppcheck)
analyze:
	@echo "🔍 Статический анализ кода"
	cppcheck --enable=all --inconclusive --std=c99 $(SOURCES)

# Создание документации (требует doxygen)
docs:
	@echo "📚 Генерация документации"
	doxygen Doxyfile

# Профилирование производительности
profile: CFLAGS += -pg
profile: $(TARGET)
	@echo "📊 Сборка с профилированием"

# Проверка утечек памяти (требует valgrind)
memcheck: debug
	@echo "🔍 Проверка утечек памяти"
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Помощь
help:
	@echo "Доступные команды:"
	@echo "  all          - Сборка в режиме отладки (по умолчанию)"
	@echo "  debug        - Сборка в режиме отладки"
	@echo "  release      - Сборка в режиме продакшена"
	@echo "  clean        - Очистка объектных файлов"
	@echo "  distclean    - Полная очистка"
	@echo "  run          - Запуск сервера"
	@echo "  run-port     - Запуск на порту PORT=xxxx"
	@echo "  check        - Проверка синтаксиса"
	@echo "  format       - Форматирование кода"
	@echo "  analyze      - Статический анализ"
	@echo "  memcheck     - Проверка утечек памяти"
	@echo "  deps-ubuntu  - Установка зависимостей Ubuntu"
	@echo "  deps-centos  - Установка зависимостей CentOS"
	@echo "  help         - Показать эту справку"

# Указание, что эти цели не являются файлами
.PHONY: all debug release clean distclean run run-port check format analyze memcheck help deps-ubuntu deps-centos archive docs profile