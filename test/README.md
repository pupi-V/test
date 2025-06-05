# Симулятор платы зарядной станции

Этот файл имитирует реальную плату зарядной станции и отправляет данные на сервер через JSON запросы.

## Использование

### Запуск одного симулятора

```bash
# Запуск slave-платы с ID 2 (по умолчанию)
node test/board-simulator.cjs

# Запуск конкретной платы
node test/board-simulator.cjs 3 slave

# Запуск master-платы
node test/board-simulator.cjs 1 master

# Запуск всех симуляторов одновременно
./test/run-multiple-simulators.sh

# Остановка всех симуляторов
./test/stop-simulators.sh
```

### Параметры

- **Первый аргумент**: ID платы (число)
- **Второй аргумент**: Тип платы (`slave` или `master`)

## Что симулирует

### Slave-плата

- **Подключение автомобиля**: Случайное подключение/отключение
- **Напряжение по фазам**: 220В ± 10В
- **Ток зарядки**: 0-32А при подключенном автомобиле
- **Мощность**: Автоматический расчет на основе напряжения и тока
- **Ошибки**: Случайные ошибки автомобиля
- **Статус**: `available`, `charging` в зависимости от состояния

### Master-плата

- **Общая мощность**: Случайные значения в пределах максимальной мощности
- **Статус**: Случайное переключение между `available`, `charging`, `maintenance`

## Формат данных

### Подключение к серверу

**Запрос:**
```json
POST /api/board/connect
{
  "boardId": 2
}
```

**Ответ для slave-платы:**
```json
{
  "id": 2,
  "type": "slave",
  "displayName": "Симулятор 2",
  "technicalName": "sim-2",
  "status": "available",
  "maxPower": 22,
  "currentPower": 0,
  "carConnection": false,
  "carChargingPermission": false,
  "carError": false,
  "masterOnline": false,
  "masterChargingPermission": false,
  "masterAvailablePower": 0,
  "voltagePhase1": 0,
  "voltagePhase2": 0,
  "voltagePhase3": 0,
  "currentPhase1": 0,
  "currentPhase2": 0,
  "currentPhase3": 0,
  "chargerPower": 0,
  "singlePhaseConnection": false,
  "powerOverconsumption": false,
  "fixedPower": false
}
```

### Обновление данных

**Запрос:**
```json
PATCH /api/stations/2
{
  "status": "charging",
  "carConnection": true,
  "voltagePhase1": 225.3,
  "voltagePhase2": 218.7,
  "voltagePhase3": 221.1,
  "currentPhase1": 15.2,
  "currentPhase2": 14.8,
  "currentPhase3": 15.5,
  "chargerPower": 10.2
}
```

## Остановка

Нажмите `Ctrl+C` для остановки симулятора.

## Логи

Симулятор выводит подробные логи о своей работе:
- Подключение к серверу
- Изменения состояния автомобиля
- Обновления данных
- Ошибки подключения