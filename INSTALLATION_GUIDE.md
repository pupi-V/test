# Руководство по установке и настройке

## Системные требования

### Для веб-приложения
- Node.js 18+ 
- npm 8+
- Минимум 2GB RAM
- Операционная система: Windows, macOS, Linux

### Для ESP32 разработки
- ESP-IDF v5.0+
- Python 3.8+
- Git
- USB драйвер для ESP32
- Минимум 4GB свободного места

## Установка веб-приложения

### 1. Клонирование проекта
```bash
git clone <repository-url>
cd charging-station-management
```

### 2. Установка зависимостей
```bash
npm install
```

### 3. Настройка окружения
Создать файл `.env` в корне проекта:
```env
NODE_ENV=development
PORT=5000
LOG_LEVEL=info
```

### 4. Запуск в режиме разработки
```bash
npm run dev
```

Приложение будет доступно по адресу: `http://localhost:5000`

### 5. Сборка для продакшна
```bash
npm run build
npm start
```

## Установка ESP-IDF для ESP32

### Windows
```powershell
# Скачать ESP-IDF Installer
# https://dl.espressif.com/dl/esp-idf-installer/

# Или через командную строку
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.bat esp32
./export.bat
```

### macOS/Linux
```bash
# Установка предварительных зависимостей
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Клонирование ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
source ./export.sh
```

### Проверка установки
```bash
idf.py --version
# Должно показать версию ESP-IDF v5.0+
```

## Настройка ESP32 проекта

### 1. Переход в папку ESP32
```bash
cd projectone
```

### 2. Настройка WiFi
Отредактировать файл `main/master_slave_logic.c`:
```c
#define WIFI_SSID      "YOUR_WIFI_NETWORK"
#define WIFI_PASS      "YOUR_WIFI_PASSWORD"
```

### 3. Конфигурация проекта (опционально)
```bash
idf.py menuconfig
```

### 4. Компиляция
```bash
idf.py build
```

### 5. Подключение ESP32
- Подключить ESP32 к компьютеру через USB
- Определить порт:
  - Windows: `COM3`, `COM4`, etc.
  - macOS/Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.

### 6. Загрузка прошивки
```bash
# Замените /dev/ttyUSB0 на ваш порт
idf.py -p /dev/ttyUSB0 flash monitor
```

## Проверка работы системы

### Веб-приложение
1. Открыть `http://localhost:5000`
2. Нажать "Сканировать платы" 
3. Проверить что симуляторы обнаруживаются

### ESP32
1. В логах найти строку с IP адресом:
   ```
   I (12345) MASTER_SLAVE: got ip:192.168.1.100
   ```
2. Открыть `http://192.168.1.100/charging-station`
3. Проверить работу веб-интерфейса

## Тестирование с симуляторами

### Запуск симуляторов
```bash
cd test

# Один симулятор
node board-simulator.cjs

# Несколько симуляторов (Linux/macOS)
./run-multiple-simulators.sh

# Остановка всех симуляторов
./stop-simulators.sh
```

### Проверка в веб-приложении
1. Открыть главную страницу
2. Нажать "Сканировать платы"
3. Должны появиться найденные симуляторы
4. Попробовать подключиться к одному из них

## Конфигурация сети

### Настройка роутера
Для корректной работы UDP коммуникации между ESP32:
- Включить multicast/broadcast трафик
- Открыть порт 3333 для UDP
- Убедиться что все устройства в одной подсети

### Настройка брандмауэра
```bash
# Linux (iptables)
sudo iptables -A INPUT -p udp --dport 3333 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 80 -j ACCEPT

# Windows Firewall
# Добавить исключения для портов 3333 (UDP) и 80 (TCP)
```

## Развертывание в продакшне

### Веб-приложение
```bash
# Сборка продакшн версии
npm run build

# Запуск через PM2
npm install -g pm2
pm2 start npm --name "charging-station" -- start

# Или через systemd (Linux)
sudo systemctl enable charging-station
sudo systemctl start charging-station
```

### ESP32 в продакшне
1. Настроить WiFi параметры для продакшн сети
2. Загрузить прошивку на все устройства
3. Настроить первое устройство как Master
4. Проверить UDP коммуникацию между устройствами

## Устранение проблем установки

### Веб-приложение

**Ошибка "EADDRINUSE"**
```bash
# Найти процесс использующий порт 5000
lsof -i :5000
# Или на Windows
netstat -ano | findstr :5000

# Завершить процесс или изменить порт в конфигурации
```

**Ошибки npm install**
```bash
# Очистка кеша npm
npm cache clean --force

# Удаление node_modules и переустановка
rm -rf node_modules package-lock.json
npm install
```

### ESP32

**ESP-IDF не найден**
```bash
# Убедиться что export.sh выполнен
source $HOME/esp/esp-idf/export.sh

# Или добавить в .bashrc/.zshrc
echo 'source $HOME/esp/esp-idf/export.sh' >> ~/.bashrc
```

**Ошибка компиляции**
```bash
# Полная очистка
idf.py fullclean

# Проверка готовности
./test_compilation_readiness.sh
```

**ESP32 не подключается**
```bash
# Проверка порта
ls /dev/tty*

# Права доступа (Linux)
sudo usermod -a -G dialout $USER
# Перелогиниться после выполнения
```

## Обновление системы

### Веб-приложение
```bash
# Обновление зависимостей
npm update

# Проверка уязвимостей
npm audit
npm audit fix
```

### ESP32
```bash
# Обновление ESP-IDF
cd $IDF_PATH
git pull
git submodule update --init --recursive
./install.sh esp32
```

## Резервное копирование

### Конфигурация станций
```bash
# Создание резервной копии данных
cp data/stations.json data/stations.backup.json

# Восстановление
cp data/stations.backup.json data/stations.json
```

### ESP32 прошивка
```bash
# Сохранение скомпилированной прошивки
cp build/charging-station.bin firmware-backup/

# Загрузка сохраненной прошивки
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x10000 firmware-backup/charging-station.bin
```

Система готова к использованию после выполнения всех шагов установки.