# Техническая документация системы управления зарядными станциями

## Архитектура системы

### Общий обзор

Система состоит из двух основных компонентов:
- **Центральное веб-приложение** - управляющий интерфейс
- **Распределенные ESP32 устройства** - физические контроллеры станций

### Схема взаимодействия

```
[Веб-браузер] ←→ [React App] ←→ [Express Server] ←→ [JSON Storage]
                                      ↓
                              [Network Scanner]
                                      ↓
                         [ESP32 Master] ←UDP→ [ESP32 Slave 1]
                              ↓                      ↓
                        [HTTP Server]         [HTTP Server]
                              ↓                      ↓
                        [Web Interface]      [Web Interface]
```

## Компоненты системы

### 1. Веб-приложение (Frontend)

**Местоположение**: `client/src/`

**Основные модули**:
- `App.tsx` - Главный компонент с маршрутизацией
- `pages/` - Страницы приложения
- `components/` - Переиспользуемые UI компоненты
- `hooks/` - Кастомные React хуки
- `lib/` - Утилиты и конфигурация

**Технологический стек**:
```typescript
// Основные зависимости
React 18.3.1
TypeScript 5.6.3
Tailwind CSS 3.4.15
TanStack Query 5.59.20
React Hook Form 7.53.2
Zod 3.23.8
```

**Ключевые компоненты**:
- `ChargingStationDashboard` - Главная панель управления
- `StationCard` - Карточка отдельной станции
- `BoardConnectionPanel` - Интерфейс подключения к ESP32
- `StationConfigForm` - Форма настройки станции

### 2. Бэкенд сервер

**Местоположение**: `server/`

**Основные файлы**:
```typescript
index.ts        // Главный сервер Express
routes.ts       // API маршруты
storage.ts      // Система хранения данных
esp32-client.ts // Клиент для работы с ESP32
vite.ts         // Конфигурация Vite в разработке
```

**API Endpoints**:
```
GET    /api/stations           # Список всех станций
GET    /api/stations/:id       # Конкретная станция
POST   /api/stations           # Создание станции
PATCH  /api/stations/:id       # Обновление станции
DELETE /api/stations/:id       # Удаление станции

POST   /api/esp32/scan         # Сканирование сети
POST   /api/esp32/connect      # Подключение к плате
POST   /api/esp32/:id/sync     # Синхронизация данных
```

### 3. ESP32 прошивка

**Местоположение**: `projectone/main/`

**Основные модули**:
```c
main.c                     // Точка входа
master_slave_logic.c       // Логика Master/Slave
udp_comm.c                 // UDP коммуникация
simple_wifi.c              // WiFi подключение
charging_station_handlers.c // HTTP API обработчики
```

**Встроенные ресурсы**:
```
charging-station.html      // Веб-интерфейс
charging-station.css       // Стили
charging-station.js        // JavaScript логика
config.json               // Конфигурация станций
```

## Протоколы коммуникации

### HTTP API (Веб ↔ ESP32)

**Формат запросов**:
```json
// GET /api/stations
{
  "stations": [
    {
      "id": 1,
      "displayName": "Станция A1",
      "technicalName": "STA_A1_001",
      "typ": "fast",
      "maxPower": 150.0,
      "currentPower": 0.0,
      "status": "available"
    }
  ]
}
```

**Обновление станции**:
```json
// PATCH /api/stations/1
{
  "displayName": "Новое имя",
  "maxPower": 200.0,
  "status": "charging"
}
```

### UDP коммуникация (ESP32 ↔ ESP32)

**Формат сообщений Master**:
```json
{
  "type": "master",
  "command": "status_request",
  "timestamp": 1703123456,
  "stations": [
    {
      "id": "ESP32_MASTER_001",
      "power": 150.0,
      "status": "online"
    }
  ]
}
```

**Формат ответов Slave**:
```json
{
  "type": "slave",
  "id": "ESP32_SLAVE_001",
  "status": "online",
  "power": 11.0,
  "stations": 2,
  "timestamp": 1703123456
}
```

## Конфигурация и настройка

### Переменные окружения

**Веб-приложение**:
```bash
NODE_ENV=development|production
PORT=5000
```

**Vite (Frontend)**:
```bash
VITE_API_BASE_URL=http://localhost:5000
```

### ESP32 конфигурация

**WiFi настройки** (`main/master_slave_logic.c`):
```c
#define WIFI_SSID      "YOUR_NETWORK"
#define WIFI_PASS      "YOUR_PASSWORD"
```

**Сетевые параметры**:
```c
#define UDP_PORT       3333
#define HTTP_PORT      80
#define MAX_STATIONS   10
```

### ESP-IDF конфигурация

**Основные параметры** (`sdkconfig.defaults`):
```ini
CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_HTTPD_MAX_REQ_HDR_LEN=1024
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
```

## Система хранения данных

### JSON Storage

**Структура файла** (`data/stations.json`):
```json
{
  "stations": [
    {
      "id": 1,
      "displayName": "Станция A1",
      "technicalName": "STA_A1_001",
      "typ": "fast|slow|ultra",
      "maxPower": 150.0,
      "currentPower": 0.0,
      "status": "available|charging|maintenance|offline",
      "location": "Парковка A, место 1",
      "lastUpdate": "2023-12-01T10:30:00Z"
    }
  ],
  "metadata": {
    "version": "1.0",
    "lastModified": "2023-12-01T10:30:00Z",
    "totalStations": 1
  }
}
```

### ESP32 конфигурация

**Файл конфигурации** (`main/config.json`):
```json
{
  "device": {
    "type": "master",
    "id": "ESP32_MASTER_001",
    "name": "Главный контроллер"
  },
  "network": {
    "udp_port": 3333,
    "http_port": 80,
    "broadcast_interval": 5000
  },
  "stations": [
    {
      "id": 1,
      "name": "Станция 1",
      "max_power": 150.0,
      "connector_type": "CCS2"
    }
  ]
}
```

## Безопасность

### Веб-приложение

- CORS настройки для локальной разработки
- Валидация данных через Zod схемы
- Санитизация пользовательского ввода
- Обработка ошибок и логирование

### ESP32

- WPA2 WiFi шифрование
- HTTP без TLS (для локальной сети)
- Валидация JSON сообщений
- Ограничение размера запросов

## Мониторинг и диагностика

### Логирование

**Веб-сервер**:
```javascript
// Формат логов Express
[timestamp] [method] [url] [status] in [time]ms :: [response_data]
```

**ESP32**:
```c
// Макросы логирования
ESP_LOGI(TAG, "Сообщение информации");
ESP_LOGW(TAG, "Предупреждение");
ESP_LOGE(TAG, "Ошибка");
```

### Метрики производительности

**Веб-приложение**:
- Время ответа API
- Использование памяти Node.js
- Количество активных соединений

**ESP32**:
- Свободная память heap
- Статус WiFi соединения
- Пропускная способность UDP
- Время ответа HTTP запросов

## Тестирование

### Симуляторы плат

**Запуск симулятора** (`test/board-simulator.cjs`):
```bash
# Один симулятор на порту 8080
node board-simulator.cjs

# Несколько симуляторов
./run-multiple-simulators.sh
```

**Конфигурация симулятора**:
```javascript
const BOARD_CONFIG = {
  type: 'master|slave',
  id: 'ESP32_SIM_001',
  port: 8080,
  stations: 2,
  maxPower: 150.0
};
```

### Unit тесты

**Структура тестов**:
```
test/
├── unit/
│   ├── storage.test.js      # Тесты хранилища
│   ├── esp32-client.test.js # Тесты ESP32 клиента
│   └── routes.test.js       # Тесты API
├── integration/
│   └── e2e.test.js         # Интеграционные тесты
└── fixtures/
    └── test-data.json      # Тестовые данные
```

## Развертывание

### Production сборка

**Веб-приложение**:
```bash
npm run build          # Сборка frontend
npm run start          # Запуск production сервера
```

**ESP32**:
```bash
cd projectone
idf.py build           # Компиляция прошивки
idf.py flash           # Загрузка на устройство
```

### Docker развертывание

**Dockerfile**:
```dockerfile
FROM node:20-alpine
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production
COPY . .
RUN npm run build
EXPOSE 5000
CMD ["npm", "start"]
```

### Масштабирование

**Горизонтальное масштабирование**:
- Балансировщик нагрузки перед веб-серверами
- Разделение по географическим зонам
- Кластеризация ESP32 устройств

**Вертикальное масштабирование**:
- Увеличение ресурсов сервера
- Оптимизация памяти ESP32
- Кеширование статических ресурсов

## Устранение неполадок

### Частые проблемы

**Веб-приложение**:
1. Порт 5000 занят - изменить в конфигурации
2. CORS ошибки - проверить настройки сервера
3. JSON ошибки - валидировать структуру данных

**ESP32**:
1. WiFi не подключается - проверить SSID/пароль
2. HTTP сервер не запускается - проверить память
3. UDP сообщения не доходят - проверить сеть

### Диагностические команды

**Сетевая диагностика**:
```bash
# Поиск ESP32 устройств
nmap -sn 192.168.1.0/24

# Проверка порта
telnet 192.168.1.100 80

# Мониторинг UDP трафика
tcpdump -i any port 3333
```

**ESP32 диагностика**:
```bash
# Мониторинг логов
idf.py monitor

# Информация о памяти
esp32-toolchain-size --format=json build/app.bin
```

## Расширение функциональности

### Добавление новых API endpoints

1. Определить схему в `shared/schema.ts`
2. Добавить маршрут в `server/routes.ts`
3. Обновить клиентскую часть
4. Протестировать с симуляторами

### Новые типы станций

1. Расширить enum в схемах
2. Обновить ESP32 конфигурацию
3. Добавить UI компоненты
4. Обновить документацию

Эта документация покрывает все аспекты системы для разработчиков и системных администраторов.