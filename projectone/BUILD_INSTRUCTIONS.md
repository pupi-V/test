# Инструкции по сборке и загрузке ESP32

## Подготовка среды разработки

### 1. Установка ESP-IDF

```bash
# Клонирование ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git

# Переход в директорию ESP-IDF
cd esp-idf

# Установка инструментов (для Linux/macOS)
./install.sh

# Настройка переменных окружения
. ./export.sh
```

### 2. Настройка проекта

```bash
# Переход в директорию проекта
cd projectone

# Настройка конфигурации
idf.py menuconfig
```

### Основные настройки в menuconfig:

1. **WiFi Configuration**:
   - Component config → WiFi
   - Установить максимальное количество соединений
   - Настроить буферы

2. **HTTP Server Configuration**:
   - Component config → HTTP Server
   - Увеличить максимальное количество URI handlers
   - Настроить размер буферов

3. **Partition Table**:
   - Partition Table → Single factory app, no OTA
   - Или Custom partition table (для больших приложений)

## Компиляция и загрузка

### 1. Сборка проекта

```bash
# Полная сборка
idf.py build

# Очистка и пересборка (при ошибках)
idf.py fullclean
idf.py build
```

### 2. Подключение ESP32

```bash
# Проверка портов (Linux)
ls /dev/ttyUSB*

# Проверка портов (macOS)
ls /dev/cu.usbserial*

# Проверка портов (Windows)
# Использовать Device Manager
```

### 3. Загрузка прошивки

```bash
# Загрузка с автоопределением порта
idf.py flash

# Загрузка на конкретный порт
idf.py -p /dev/ttyUSB0 flash

# Загрузка и мониторинг
idf.py -p /dev/ttyUSB0 flash monitor
```

### 4. Мониторинг

```bash
# Только мониторинг
idf.py monitor

# Выход из мониторинга: Ctrl+]
```

## Настройка WiFi

### В коде (master_slave_logic.c):

```c
#define WIFI_SSID      "YourWiFiNetwork"
#define WIFI_PASS      "YourPassword"
```

### Или через config.json:

```json
{
  "wifi": {
    "ssid": "YourWiFiNetwork",
    "password": "YourPassword"
  },
  "device": {
    "type": "master",
    "id": "ESP32_MASTER_001"
  }
}
```

## Доступ к веб интерфейсу

После успешной загрузки и подключения к WiFi:

1. Найти IP адрес ESP32 в логах:
   ```
   I (xxxx) MASTER_SLAVE: Получен IP:192.168.1.100
   ```

2. Открыть в браузере:
   - Современный интерфейс: `http://192.168.1.100/charging-station`
   - Классический интерфейс: `http://192.168.1.100/`

## API эндпоинты

- `GET /api/stations` - Список зарядных станций
- `POST /api/stations/{id}` - Обновление станции
- `POST /api/esp32/scan` - Сканирование плат
- `POST /api/esp32/connect` - Подключение к плате

## Устранение проблем

### Ошибки компиляции:

1. **Недостаточно памяти**:
   ```bash
   idf.py menuconfig
   → Component config → ESP32-specific → Main XTAL frequency → 40 MHz
   ```

2. **Ошибки включения файлов**:
   - Проверить пути в CMakeLists.txt
   - Убедиться, что все .h файлы существуют

3. **Ошибки линковки**:
   - Проверить секцию REQUIRES в CMakeLists.txt
   - Добавить недостающие компоненты

### Ошибки загрузки:

1. **Не найден порт**:
   ```bash
   # Проверить подключение USB
   # Установить драйверы CP210x или CH340
   ```

2. **Ошибка разрешений (Linux)**:
   ```bash
   sudo usermod -a -G dialout $USER
   # Перезагрузиться
   ```

3. **Bootloader не отвечает**:
   - Зажать кнопку BOOT при подключении питания
   - Проверить качество USB кабеля

### Ошибки WiFi:

1. **Не подключается к сети**:
   - Проверить SSID и пароль
   - Убедиться, что сеть работает на 2.4 GHz
   - Проверить дальность от роутера

2. **Получает IP, но веб сервер не отвечает**:
   - Проверить брандмауэр
   - Пропинговать IP адрес ESP32
   - Проверить логи на предмет ошибок

## Отладка

### Уровни логирования в menuconfig:

```
Component config → Log output
→ Default log verbosity → Debug
```

### Дополнительные логи в коде:

```c
ESP_LOGI(TAG, "Отладочное сообщение");
ESP_LOGD(TAG, "Детальная отладка");
ESP_LOGE(TAG, "Ошибка: %s", error_msg);
```

## Конфигурация типа устройства

### Master устройство:
```c
device_type_t device_type = DEVICE_TYPE_MASTER;
```

### Slave устройство:
```c
device_type_t device_type = DEVICE_TYPE_SLAVE;
```

Или настроить через конфигурационный файл config.json.

## Требования к оборудованию

- ESP32 DevKit или аналогичная плата
- USB кабель для программирования
- WiFi роутер 2.4 GHz
- Минимум 4MB Flash памяти
- Рекомендуется: ESP32 с внешней антенной для лучшего WiFi приема

Проект готов к компиляции и загрузке на ESP32!