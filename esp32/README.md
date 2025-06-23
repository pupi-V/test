# ESP32 Charging Station Interface

Автономный веб-интерфейс для ESP32 платы зарядной станции.

## Установка и прошивка

### Способ 1: Arduino IDE
1. Установите Arduino IDE
2. Добавьте ESP32 board package:
   - File → Preferences → Additional Board Manager URLs
   - Добавьте: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → найдите "ESP32" и установите

3. Установите библиотеки:
   - Tools → Manage Libraries
   - Найдите и установите "ArduinoJson" by Benoit Blanchon

4. Настройки платы:
   - Board: "ESP32 Dev Module"
   - Upload Speed: 921600
   - CPU Frequency: 240MHz
   - Flash Size: 4MB

### Способ 2: PlatformIO
1. Установите VSCode и расширение PlatformIO
2. Откройте папку esp32 как проект PlatformIO
3. Прошейте командой: `pio run --target upload`

## Настройка WiFi

Отредактируйте файл `main.cpp`:
```cpp
const char* ssid = "ВАШ_WIFI_SSID";
const char* password = "ВАШ_WIFI_ПАРОЛЬ";
```

## Использование

1. Прошейте ESP32
2. Подключите к WiFi
3. Откройте Serial Monitor (115200 baud)
4. Найдите IP адрес в логах
5. Откройте в браузере: `http://IP_АДРЕС`

## API Endpoints

- `GET /` - Веб-интерфейс
- `GET /api/station` - Данные станции (JSON)
- `GET /api/info` - Информация о плате
- `POST /api/update` - Обновление данных

## Функции

- Отображение статуса зарядной станции
- Мониторинг параметров в реальном времени
- Автообновление данных каждые 5 секунд
- Совместимость с основным интерфейсом
- mDNS поддержка (charging-station.local)

## Совместимость

Этот код совместим с основным Node.js приложением через API. ESP32 может работать как slave устройство в системе управления зарядными станциями.