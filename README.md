# EV Charging Station Management System

Высокопроизводительное веб-приложение для управления зарядными станциями с C-бэкендом и React фронтендом.

## Быстрый запуск

```bash
# Установка зависимостей фронтенда
npm install

# Запуск C-сервера
npm run dev

# Или ручной запуск
cd server_c && make debug && ./charging_station_server
```

## Доступ к приложению

После запуска приложение доступно по адресам:
- **Локально**: http://localhost:5000
- **В сети**: http://YOUR_IP:5000

## Настройка для внешнего доступа

### Windows Firewall
1. Откройте "Параметры Windows" → "Безопасность Windows"
2. "Брандмауэр и защита сети" → "Разрешить работу с приложением"
3. Добавьте Node.js или разрешите порт 5000

### Переменные окружения
Создайте файл `.env`:
```
PORT=5000
HOST=0.0.0.0
NODE_ENV=development
```

## Развертывание на ESP32

### Быстрый старт ESP32
1. Откройте файл `esp32/charging_station.ino` в Arduino IDE
2. Измените WiFi настройки:
   ```cpp
   const char* ssid = "ВАШ_WIFI";
   const char* password = "ВАШ_ПАРОЛЬ";
   ```
3. Установите библиотеку ArduinoJson
4. Загрузите код на ESP32
5. Откройте Serial Monitor для получения IP адреса
6. Перейдите по адресу `http://IP_АДРЕС`

### Требования для ESP32
- ESP32 Dev Module
- Arduino IDE с ESP32 board package
- Библиотека ArduinoJson

## Возможности

- Управление зарядными станциями
- Мониторинг в реальном времени  
- Поддержка master/slave архитектуры
- Сканирование ESP32 устройств в сети
- Автономный веб-интерфейс на ESP32
- Темная и светлая темы

## API

- `GET /api/stations` - Список станций
- `POST /api/stations` - Создать станцию
- `PATCH /api/stations/:id` - Обновить станцию
- `DELETE /api/stations/:id` - Удалить станцию
- `POST /api/esp32/scan` - Сканировать ESP32 устройства