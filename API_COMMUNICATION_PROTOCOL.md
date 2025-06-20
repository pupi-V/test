# API Communication Protocol: Web Interface ↔ ESP32 Boards

Полная схема запросов, которые отправляет веб-интерфейс к платам ESP32 и ожидаемые ответы.

## Запросы к SLAVE-плате

### 1. Обнаружение платы
**Запрос:**
```http
GET http://192.168.1.100:80/api/info
Content-Type: application/json
```

**Ожидаемый ответ:**
```json
{
  "device_type": "charging_station",
  "board_type": "slave",
  "board_id": "ESP32_SLAVE_001",
  "display_name": "Зарядная станция №1",
  "technical_name": "CS-SLAVE-001",
  "max_power": 22.0
}
```

### 2. Подключение к плате
**Запрос:**
```http
POST http://192.168.1.100:80/api/connect
Content-Type: application/json

{
  "client_type": "web_interface",
  "timestamp": 1703851200000
}
```

**Ожидаемый ответ:**
```json
{
  "status": "connected",
  "message": "Подключение успешно",
  "timestamp": 1703851200000
}
```

### 3. Получение данных станции
**Запрос:**
```http
GET http://192.168.1.100:80/api/data
Content-Type: application/json
```

**Ожидаемый ответ:**
```json
{
  "status": "charging",
  "current_power": 15.5,
  "slave_data": {
    "car_connected": true,
    "car_charging_permission": true,
    "car_error": false,
    "voltage_phase1": 230.5,
    "voltage_phase2": 229.8,
    "voltage_phase3": 231.2,
    "current_phase1": 16.2,
    "current_phase2": 15.8,
    "current_phase3": 16.5,
    "charger_power": 15.5,
    "single_phase_connection": false,
    "power_overconsumption": false,
    "fixed_power": true
  },
  "master_connection": {
    "master_online": true,
    "master_charging_permission": true,
    "master_available_power": 100.0,
    "last_master_contact": "2025-06-20T10:48:00Z"
  }
}
```

### 4. Отправка команд управления
**Запрос:**
```http
POST http://192.168.1.100:80/api/data
Content-Type: application/json

{
  "command": "update_settings",
  "settings": {
    "charging_permission": true,
    "max_power": 20.0,
    "fixed_power": false
  }
}
```

**Ожидаемый ответ:**
```json
{
  "status": "success",
  "message": "Настройки обновлены",
  "applied_settings": {
    "charging_permission": true,
    "max_power": 20.0,
    "fixed_power": false
  },
  "current_state": {
    "status": "charging",
    "current_power": 18.5,
    "car_connected": true
  }
}
```

## Запросы к MASTER-плате

### 1. Обнаружение платы
**Запрос:**
```http
GET http://192.168.1.50:80/api/info
Content-Type: application/json
```

**Ожидаемый ответ:**
```json
{
  "device_type": "charging_station",
  "board_type": "master",
  "board_id": "ESP32_MASTER_001",
  "display_name": "Главный контроллер",
  "technical_name": "CS-MASTER-001",
  "max_power": 250.0
}
```

### 2. Подключение к master-плате
**Запрос:**
```http
POST http://192.168.1.50:80/api/connect
Content-Type: application/json

{
  "client_type": "web_interface",
  "timestamp": 1703851200000
}
```

**Ожидаемый ответ:**
```json
{
  "status": "connected",
  "message": "Master контроллер подключен",
  "timestamp": 1703851200000,
  "connected_slaves": 3
}
```

### 3. Получение статуса всех станций
**Запрос:**
```http
GET http://192.168.1.50:80/api/data
Content-Type: application/json
```

**Ожидаемый ответ:**
```json
{
  "master_info": {
    "status": "available",
    "current_power": 45.3,
    "total_stations": 3,
    "active_stations": 2
  },
  "stations": [
    {
      "id": 1,
      "station_id": "CS-SLAVE-001",
      "ip": "192.168.1.100",
      "status": "charging",
      "current_power": 15.5,
      "max_power": 22.0,
      "last_update": "2025-06-20T10:48:00Z",
      "slave_data": {
        "car_connected": true,
        "car_charging_permission": true,
        "car_error": false,
        "voltage_phase1": 230.5,
        "voltage_phase2": 229.8,
        "voltage_phase3": 231.2,
        "current_phase1": 16.2,
        "current_phase2": 15.8,
        "current_phase3": 16.5,
        "charger_power": 15.5
      }
    },
    {
      "id": 2,
      "station_id": "CS-SLAVE-002",
      "ip": "192.168.1.101",
      "status": "available",
      "current_power": 0.0,
      "max_power": 22.0,
      "last_update": "2025-06-20T10:47:55Z",
      "slave_data": {
        "car_connected": false,
        "car_charging_permission": false,
        "car_error": false,
        "voltage_phase1": 230.0,
        "voltage_phase2": 230.0,
        "voltage_phase3": 230.0,
        "current_phase1": 0.0,
        "current_phase2": 0.0,
        "current_phase3": 0.0,
        "charger_power": 0.0
      }
    }
  ],
  "network_info": {
    "total_power_consumption": 15.5,
    "available_power": 234.5,
    "power_distribution": "balanced"
  }
}
```

### 4. Управление slave-станциями через master
**Запрос:**
```http
POST http://192.168.1.50:80/api/data
Content-Type: application/json

{
  "command": "control_slave",
  "target_slave": "CS-SLAVE-001",
  "action": "update_settings",
  "settings": {
    "charging_permission": false,
    "max_power": 18.0
  }
}
```

**Ожидаемый ответ:**
```json
{
  "status": "success",
  "message": "Команда отправлена на CS-SLAVE-001",
  "target_slave": "CS-SLAVE-001",
  "command_sent": true,
  "response_from_slave": {
    "status": "success",
    "settings_applied": true,
    "new_status": "available"
  }
}
```

## Периодичность запросов

1. **Сканирование сети**: каждые 2-3 минуты
2. **Получение данных**: каждые 5 секунд для активных станций
3. **Команды управления**: по требованию пользователя
4. **Проверка подключения**: при потере связи

## Обработка ошибок

**При отсутствии ответа:**
```json
{
  "error": "Плата по адресу 192.168.1.100 не отвечает или недоступна"
}
```

**При ошибке формата:**
```json
{
  "error": "Неверный формат данных от платы",
  "received_data": "..."
}
```

## Статусы станций

- **available**: Станция доступна для зарядки
- **charging**: Происходит зарядка автомобиля
- **offline**: Станция не в сети или неисправна
- **maintenance**: Станция на обслуживании

## Ключевые поля данных

### Для всех плат:
- `device_type`: всегда "charging_station"
- `board_type`: "master" или "slave"
- `status`: текущий статус станции
- `current_power`: текущая мощность в кВт

### Для slave-плат:
- `car_connected`: подключен ли автомобиль
- `voltage_phase1/2/3`: напряжение по фазам (В)
- `current_phase1/2/3`: ток по фазам (А)
- `charger_power`: мощность зарядки (кВт)
- `master_online`: связь с master-платой

### Для master-плат:
- `total_stations`: общее количество станций
- `active_stations`: количество активных станций
- `stations`: массив данных всех slave-станций
- `network_info`: информация о сети и распределении мощности

Эта схема обеспечивает полную совместимость между веб-интерфейсом и ESP32 платами для управления системой зарядных станций.