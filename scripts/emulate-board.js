#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

const dataPath = path.join(__dirname, '..', 'data', 'stations.json');

// Шаблоны плат для эмуляции
const boardTemplates = {
  master: {
    "displayName": "Master Control Unit",
    "technicalName": "master-001",
    "type": "master",
    "maxPower": 250,
    "status": "online",
    "ipAddress": "192.168.1.100",
    "description": "Главная управляющая плата системы",
    "id": 1,
    "currentPower": 15.2
  },
  slave: {
    "displayName": "Charging Station Unit",
    "technicalName": "slave-001",
    "type": "slave",
    "maxPower": 22,
    "status": "available",
    "ipAddress": "192.168.1.101",
    "description": "Зарядная станция подчиненного типа",
    "id": 2,
    "currentPower": 0,
    "carConnection": false,
    "carChargingPermission": false,
    "carError": false,
    "masterOnline": true,
    "masterChargingPermission": true,
    "masterAvailablePower": 50,
    "voltagePhase1": 220.5,
    "voltagePhase2": 219.8,
    "voltagePhase3": 221.2,
    "currentPhase1": 0,
    "currentPhase2": 0,
    "currentPhase3": 0,
    "chargerPower": 0,
    "singlePhaseConnection": false,
    "powerOverconsumption": false,
    "fixedPower": false
  }
};

function emulateBoard(boardType, boardId = null) {
  try {
    let stations = [];
    
    // Пытаемся загрузить существующие данные
    try {
      const data = fs.readFileSync(dataPath, 'utf-8');
      stations = JSON.parse(data);
    } catch (error) {
      console.log('Создаю новый файл данных...');
    }

    switch (boardType) {
      case 'master':
        // Удаляем все существующие master платы
        stations = stations.filter(s => s.type !== 'master');
        // Добавляем новую master плату
        const masterBoard = { ...boardTemplates.master };
        if (boardId) masterBoard.id = parseInt(boardId);
        stations.unshift(masterBoard);
        console.log(`Эмулируется подключение Master платы (ID: ${masterBoard.id})`);
        break;

      case 'slave':
        // Удаляем все master платы, оставляем только slave
        stations = stations.filter(s => s.type !== 'master');
        // Если нет slave плат, добавляем одну
        if (stations.length === 0) {
          const slaveBoard = { ...boardTemplates.slave };
          if (boardId) slaveBoard.id = parseInt(boardId);
          stations.push(slaveBoard);
        }
        console.log(`Эмулируется подключение Slave платы (ID: ${stations[0].id})`);
        break;

      case 'none':
        // Очищаем все данные
        stations = [];
        console.log('Эмулируется отсутствие подключенных плат');
        break;

      case 'both':
        // Добавляем и master и slave платы
        stations = [];
        const master = { ...boardTemplates.master };
        const slave = { ...boardTemplates.slave };
        slave.id = 2;
        stations.push(master, slave);
        console.log('Эмулируется подключение Master и Slave плат');
        break;

      default:
        console.error('Неизвестный тип платы. Используйте: master, slave, none, both');
        process.exit(1);
    }

    // Создаем папку если не существует
    const dataDir = path.dirname(dataPath);
    if (!fs.existsSync(dataDir)) {
      fs.mkdirSync(dataDir, { recursive: true });
    }

    // Сохраняем данные
    fs.writeFileSync(dataPath, JSON.stringify(stations, null, 2));
    console.log(`Данные сохранены в ${dataPath}`);
    console.log(`Количество станций: ${stations.length}`);
    
    if (stations.length > 0) {
      console.log('\nСписок станций:');
      stations.forEach(station => {
        console.log(`  ID ${station.id}: ${station.type.toUpperCase()} - ${station.displayName || 'Без названия'}`);
      });
    }

  } catch (error) {
    console.error('Ошибка при эмуляции платы:', error.message);
    process.exit(1);
  }
}

// Обработка аргументов командной строки
const args = process.argv.slice(2);
if (args.length === 0) {
  console.log(`
Эмулятор подключения плат зарядных станций

Использование:
  node scripts/emulate-board.js <тип> [id]

Типы плат:
  master  - Главная управляющая плата (переход к dashboard)
  slave   - Подчиненная плата станции (переход к интерфейсу станции)
  none    - Отсутствие плат (остается на экране ожидания)
  both    - И master и slave платы

Примеры:
  node scripts/emulate-board.js master
  node scripts/emulate-board.js slave 5
  node scripts/emulate-board.js none
  node scripts/emulate-board.js both
`);
  process.exit(0);
}

const boardType = args[0];
const boardId = args[1];

emulateBoard(boardType, boardId);