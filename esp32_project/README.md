# ESP32 Система управления зарядными станциями для электромобилей

## Описание проекта

Полнофункциональная система управления зарядными станциями для ESP32 модулей с 16MB Flash памятью. Включает веб-интерфейс, WebSocket поддержку для real-time обновлений и возможность управления до 50 зарядными станциями.

## Возможности системы

### Основные функции:
- 🌐 **Полнофункциональный веб-интерфейс** - современный адаптивный интерфейс
- ⚡ **Real-time обновления** - данные обновляются каждые 5 секунд через WebSocket
- 📊 **Управление станциями** - добавление, редактирование, удаление станций
- 🔌 **Мониторинг параметров** - напряжение, ток, мощность по фазам
- 📁 **LittleFS файловая система** - надежное хранение данных
- 🔄 **OTA обновления** - обновление прошивки по воздуху
- 📱 **Адаптивный дизайн** - работает на всех устройствах

### Технические характеристики:
- **Максимум станций**: 50
- **Одновременные подключения**: 20+
- **Обновление данных**: каждые 5 секунд
- **Файловая система**: LittleFS
- **Протоколы**: HTTP, WebSocket, mDNS

## Требования к оборудованию

### ESP32 модуль:
- **Flash память**: минимум 16MB (рекомендуется)
- **RAM**: 512KB (стандартно для ESP32)
- **WiFi**: встроенный модуль ESP32

### Рекомендуемые модули:
- ESP32-WROVER-IE (16MB Flash + 8MB PSRAM)
- ESP32-S3-DevKitC-1 (16MB Flash)
- Любой ESP32 с 16MB Flash памятью

## Установка и настройка

### 1. Подготовка среды разработки

#### Установка Visual Studio Code:
1. **Скачайте VS Code** с официального сайта: https://code.visualstudio.com/
2. **Установите расширение PlatformIO**:
   - Откройте VS Code
   - Перейдите в раздел Extensions (Ctrl+Shift+X)
   - Найдите "PlatformIO IDE" и установите
   - Перезапустите VS Code

#### Настройка PlatformIO:
1. **Откройте PlatformIO Home** (значок домика в левой панели)
2. **Проверьте установку платформы ESP32**:
   - Перейдите в "Platforms"
   - Найдите "Espressif 32" и убедитесь, что установлена последняя версия

### 2. Подготовка проекта

#### Клонирование проекта:
```bash
# Скопируйте папку esp32_project в удобное место
# Например: C:\ESP32_Projects\charging_stations\
```

#### Открытие проекта в VS Code:
1. **Откройте VS Code**
2. **File → Open Folder**
3. **Выберите папку esp32_project**
4. **PlatformIO автоматически определит проект**

### 3. Настройка WiFi сети

Откройте файл `src/main.cpp` и измените настройки WiFi:

```cpp
// Настройки WiFi сети
const char* ssid = "ESP32_ChargingStations";      // Название сети
const char* password = "12345678";                // Пароль сети
```

**Варианты подключения:**

#### Режим точки доступа (по умолчанию):
- ESP32 создает собственную WiFi сеть
- Подключайтесь к сети "ESP32_ChargingStations"
- Пароль: "12345678"
- IP адрес: 192.168.4.1

#### Режим клиента (подключение к существующей сети):
```cpp
WiFi.mode(WIFI_STA);                    // Режим клиента
WiFi.begin("Название_вашей_сети", "пароль");
```

### 4. Сборка и загрузка прошивки

#### Подключение ESP32:
1. **Подключите ESP32** к компьютеру через USB
2. **Установите драйверы** (обычно устанавливаются автоматически)
3. **Проверьте COM порт** в Device Manager

#### Сборка проекта:
1. **Откройте терминал PlatformIO** (Terminal → New Terminal)
2. **Выполните команду сборки**:
```bash
pio run
```

#### Загрузка прошивки:
1. **Зажмите кнопку BOOT** на ESP32 (если есть)
2. **Выполните команду загрузки**:
```bash
pio run --target upload
```
3. **Дождитесь завершения загрузки**

#### Альтернативный способ через интерфейс:
1. **Нажмите на значок PlatformIO** в левой панели
2. **В разделе PROJECT TASKS найдите esp32dev**
3. **Нажмите "Build"** для сборки
4. **Нажмите "Upload"** для загрузки

### 5. Загрузка веб-файлов

#### Подготовка файловой системы:
```bash
# Сборка файловой системы
pio run --target buildfs

# Загрузка файлов в ESP32
pio run --target uploadfs
```

#### Проверка загрузки:
1. **Откройте Serial Monitor**:
```bash
pio device monitor
```
2. **Перезагрузите ESP32** (кнопка Reset)
3. **Проверьте логи инициализации**

### 6. Первый запуск

#### Подключение к системе:
1. **Подключитесь к WiFi сети** "ESP32_ChargingStations" (пароль: 12345678)
2. **Откройте браузер** и перейдите по адресу: http://192.168.4.1
3. **Или используйте mDNS**: http://chargingstations.local

#### Проверка работоспособности:
- ✅ Веб-интерфейс загружается
- ✅ WebSocket подключение активно
- ✅ Отображаются тестовые станции
- ✅ Данные обновляются каждые 5 секунд

## Использование системы

### Основные функции веб-интерфейса:

#### Просмотр станций:
- **Статус станций** - доступна, заряжает, ошибка, отключена
- **Параметры мощности** - максимальная, текущая, доступная
- **Электрические параметры** - напряжение и ток по фазам
- **Состояние подключения** - автомобиль подключен/отключен

#### Управление станциями:
- **Добавление станции** - кнопка "➕ Добавить станцию"
- **Редактирование** - кнопка "✏️ Изменить" на карточке станции
- **Удаление** - кнопка "🗑️ Удалить" на карточке станции
- **Экспорт данных** - кнопка "📊 Экспорт данных"

#### Мониторинг системы:
- **Статус WebSocket** - отображается в верхней панели
- **Количество станций** - автоматически обновляется
- **Время последнего обновления** - показывает актуальность данных

### Аварийные функции:
- **Аварийная остановка** - кнопка "🛑 Аварийная остановка"
- **Обновление данных** - кнопка "🔄 Обновить данные"

## Отладка и диагностика

### Просмотр логов:
```bash
# Подключение к Serial Monitor
pio device monitor

# Установка скорости 115200 baud
```

### Типичные проблемы и решения:

#### ESP32 не подключается:
1. **Проверьте драйверы** CH340/CP2102
2. **Попробуйте другой USB кабель**
3. **Зажмите кнопку BOOT** при загрузке
4. **Проверьте правильность COM порта**

#### Ошибка загрузки файловой системы:
1. **Проверьте размер Flash памяти** (должно быть 16MB)
2. **Убедитесь в правильности partition table**
3. **Попробуйте стереть Flash**: `pio run --target erase`

#### Веб-интерфейс не загружается:
1. **Проверьте подключение к WiFi**
2. **Убедитесь, что файлы загружены** (`uploadfs`)
3. **Проверьте IP адрес** в Serial Monitor
4. **Попробуйте http://chargingstations.local**

#### WebSocket не подключается:
1. **Проверьте firewall** на компьютере
2. **Убедитесь в стабильности WiFi**
3. **Перезагрузите ESP32**
4. **Проверьте логи в браузере** (F12 → Console)

### Мониторинг производительности:
```cpp
// В Serial Monitor отображается:
// - Использование памяти
// - Количество подключений
// - Статус файловой системы
// - Ошибки и предупреждения
```

## Расширение функциональности

### Добавление новых API endpoints:
```cpp
// В функции setupAPIRoutes() добавьте:
server.on("/api/custom", HTTP_GET, [](AsyncWebServerRequest *request) {
  // Ваш код здесь
});
```

### Интеграция с внешними системами:
- **MQTT** - для интеграции с IoT платформами
- **HTTP API** - для подключения к внешним сервисам
- **Modbus** - для связи с промышленным оборудованием

### Расширение веб-интерфейса:
- Файлы интерфейса находятся в `data/www/`
- Используйте современные веб-технологии
- Добавляйте новые страницы и функции

## Техническая поддержка

### Полезные ссылки:
- **PlatformIO документация**: https://docs.platformio.org/
- **ESP32 документация**: https://docs.espressif.com/
- **LittleFS**: https://github.com/littlefs-project/littlefs

### Контакты для поддержки:
- Создавайте issues в репозитории проекта
- Используйте форумы PlatformIO и ESP32

---

**Удачи в использовании системы управления зарядными станциями на ESP32! 🔌⚡**