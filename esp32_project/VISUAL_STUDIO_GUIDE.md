# 🔌 Загрузка ESP32 системы через Visual Studio Code

## 📖 Пошаговая инструкция

### 1️⃣ Подготовка Visual Studio Code

#### Установка VS Code:
1. **Скачайте VS Code** с https://code.visualstudio.com/
2. **Установите программу** со стандартными настройками
3. **Запустите VS Code**

#### Установка расширения PlatformIO:
1. **Откройте панель расширений**: `Ctrl+Shift+X`
2. **Найдите "PlatformIO IDE"** 
3. **Нажмите "Install"**
4. **Перезапустите VS Code** после установки

### 2️⃣ Открытие проекта

1. **Скопируйте папку `esp32_project`** в удобное место
2. **В VS Code**: `File → Open Folder`
3. **Выберите папку `esp32_project`**
4. **PlatformIO автоматически определит проект**

### 3️⃣ Настройка WiFi

Откройте `src/main.cpp` и измените настройки:

```cpp
// Режим точки доступа (рекомендуется)
const char* ssid = "ESP32_ChargingStations";
const char* password = "12345678";

// Или подключение к существующей сети:
// const char* ssid = "Ваша_WiFi_сеть";
// const char* password = "пароль_сети";
```

### 4️⃣ Подключение ESP32

1. **Подключите ESP32** через USB кабель
2. **Установите драйверы** (если нужно):
   - CH340: https://sparks.gogo.co.nz/ch340.html
   - CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

### 5️⃣ Сборка и загрузка через VS Code

#### Способ 1: Через интерфейс PlatformIO

1. **Нажмите на иконку PlatformIO** в левой панели (муравей)
2. **Найдите "esp32dev" в PROJECT TASKS**
3. **Выполните по порядку**:
   - `General → Build` - сборка прошивки
   - `Platform → Build Filesystem Image` - сборка файловой системы
   - `General → Upload` - загрузка прошивки
   - `Platform → Upload Filesystem Image` - загрузка веб-файлов

#### Способ 2: Через терминал VS Code

1. **Откройте терминал**: `Terminal → New Terminal`
2. **Выполните команды**:

```bash
# Сборка проекта
pio run

# Сборка файловой системы
pio run --target buildfs

# Загрузка прошивки
pio run --target upload

# Загрузка файлов веб-интерфейса
pio run --target uploadfs
```

#### Способ 3: Автоматический скрипт

```bash
# Автоматическая сборка и загрузка
python scripts/build_and_upload.py

# С указанием порта
python scripts/build_and_upload.py --port COM3

# Только сборка
python scripts/build_and_upload.py --only-build
```

### 6️⃣ Мониторинг загрузки

#### Стандартный Serial Monitor:
1. **В PlatformIO**: `esp32dev → Monitor`
2. **Или в терминале**: `pio device monitor`

#### Расширенный монитор с цветами:
```bash
python scripts/monitor.py
python scripts/monitor.py --filter success
python scripts/monitor.py --save-log
```

### 7️⃣ Проверка работы

После успешной загрузки в Serial Monitor появится:

```
=== ESP32 Charging Station Management System ===
✓ LittleFS инициализирована
✓ WiFi точка доступа создана
IP адрес: 192.168.4.1
✓ mDNS запущен: http://chargingstations.local
✓ Веб-сервер запущен на порту 80
✓ Созданы тестовые станции
=== Система готова к работе ===
```

### 8️⃣ Доступ к веб-интерфейсу

1. **Подключитесь к WiFi**: "ESP32_ChargingStations" (пароль: 12345678)
2. **Откройте браузер**: http://192.168.4.1
3. **Или используйте**: http://chargingstations.local

## 🛠️ Решение проблем

### ❌ ESP32 не определяется в Device Manager

**Решения**:
1. Установите драйверы CH340 или CP2102
2. Попробуйте другой USB кабель
3. Убедитесь, что кабель поддерживает передачу данных

### ❌ Ошибка "A fatal error occurred: Failed to connect"

**Решения**:
1. **Зажмите кнопку BOOT** на ESP32
2. **Запустите загрузку**
3. **Отпустите BOOT** после начала загрузки

### ❌ Недостаточно места в Flash памяти

**Решения**:
1. Убедитесь, что ESP32 имеет 16MB Flash
2. Проверьте настройки в `platformio.ini`:
```ini
board_build.partitions = huge_app.csv
board_build.filesystem = littlefs
```

### ❌ Веб-интерфейс не загружается

**Решения**:
1. Убедитесь, что выполнили `Upload Filesystem Image`
2. Проверьте подключение к правильной WiFi сети
3. Попробуйте http://192.168.4.1 вместо доменного имени

## 📊 Горячие клавиши VS Code + PlatformIO

| Действие | Горячие клавиши |
|----------|----------------|
| Сборка | `Ctrl+Alt+B` |
| Загрузка | `Ctrl+Alt+U` |
| Очистка | `Ctrl+Alt+C` |
| Serial Monitor | `Ctrl+Alt+S` |
| Терминал | `Ctrl+Shift+\`` |

## 🎯 Дополнительные возможности

### OTA обновления (Over-The-Air):
После первой загрузки можно обновлять прошивку по WiFi:
```bash
pio run --target upload --upload-port 192.168.4.1
```

### Отладка через WiFi:
```cpp
// В коде добавьте для remote debug
#include <ESPAsyncTCP.h>
// Настройка remote logging
```

### Мониторинг производительности:
```bash
# Просмотр использования памяти
python scripts/monitor.py --filter memory

# Анализ сетевых подключений  
python scripts/monitor.py --filter network
```

---

## ✅ Финальная проверка

После успешной загрузки проверьте:

- ✅ **Serial Monitor показывает успешный запуск**
- ✅ **WiFi сеть "ESP32_ChargingStations" видна**
- ✅ **Веб-интерфейс доступен по адресу 192.168.4.1**
- ✅ **WebSocket подключение активно (зеленый статус)**
- ✅ **Отображаются тестовые станции**
- ✅ **Данные обновляются каждые 5 секунд**

**Поздравляем! Система управления зарядными станциями успешно развернута на ESP32!** 🎉