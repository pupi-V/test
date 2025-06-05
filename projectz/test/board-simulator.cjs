#!/usr/bin/env node

/**
 * Симулятор платы зарядной станции
 * Имитирует подключение реальной платы и отправку данных на сервер
 */

const http = require('http');

class BoardSimulator {
  constructor(boardId, type = 'slave') {
    this.boardId = boardId;
    this.type = type;
    this.serverUrl = 'http://localhost:5000';
    this.isConnected = false;
    this.updateInterval = null;
    
    // Начальные данные платы
    this.boardData = this.initializeBoardData();
  }

  /**
   * Инициализация данных платы в зависимости от типа
   */
  initializeBoardData() {
    const baseData = {
      displayName: `Симулятор ${this.boardId}`,
      technicalName: `sim-${this.boardId}`,
      maxPower: this.type === 'master' ? 220 : 22,
      currentPower: 0,
      status: 'available'
    };

    if (this.type === 'slave') {
      return {
        ...baseData,
        // Секция Car
        carConnection: false,
        carChargingPermission: false,
        carError: false,
        
        // Секция Master (читается от master-платы)
        masterOnline: false,
        masterChargingPermission: false,
        masterAvailablePower: 0,
        
        // Секция Charger
        voltagePhase1: 0,
        voltagePhase2: 0,
        voltagePhase3: 0,
        currentPhase1: 0,
        currentPhase2: 0,
        currentPhase3: 0,
        chargerPower: 0,
        singlePhaseConnection: false,
        powerOverconsumption: false,
        fixedPower: false
      };
    }

    return baseData;
  }

  /**
   * Подключение к серверу
   */
  async connect() {
    try {
      console.log(`[Плата ${this.boardId}] Попытка подключения к серверу...`);
      
      const response = await this.makeRequest('POST', '/api/board/connect', {
        boardId: this.boardId
      });

      if (response.success) {
        this.isConnected = true;
        console.log(`[Плата ${this.boardId}] Успешно подключена как ${response.data.type}`);
        console.log(`[Плата ${this.boardId}] Данные:`, JSON.stringify(response.data, null, 2));
        
        // Запускаем симуляцию данных
        this.startSimulation();
      }
    } catch (error) {
      console.error(`[Плата ${this.boardId}] Ошибка подключения:`, error.message);
    }
  }

  /**
   * Запуск симуляции изменения данных
   */
  startSimulation() {
    console.log(`[Плата ${this.boardId}] Запуск симуляции данных...`);
    
    this.updateInterval = setInterval(() => {
      this.simulateDataChanges();
      this.sendDataUpdate();
    }, 3000); // Обновляем данные каждые 3 секунды
  }

  /**
   * Симуляция изменения данных платы
   */
  simulateDataChanges() {
    if (this.type === 'slave') {
      // Имитируем подключение/отключение автомобиля
      if (Math.random() < 0.3) {
        this.boardData.carConnection = !this.boardData.carConnection;
        console.log(`[Плата ${this.boardId}] Автомобиль ${this.boardData.carConnection ? 'подключен' : 'отключен'}`);
      }

      // Имитируем изменение напряжения
      this.boardData.voltagePhase1 = 220 + (Math.random() - 0.5) * 20;
      this.boardData.voltagePhase2 = 220 + (Math.random() - 0.5) * 20;
      this.boardData.voltagePhase3 = 220 + (Math.random() - 0.5) * 20;

      // Имитируем ток зарядки
      if (this.boardData.carConnection) {
        this.boardData.currentPhase1 = Math.random() * 32;
        this.boardData.currentPhase2 = Math.random() * 32;
        this.boardData.currentPhase3 = Math.random() * 32;
        this.boardData.chargerPower = (this.boardData.voltagePhase1 * this.boardData.currentPhase1 + 
                                      this.boardData.voltagePhase2 * this.boardData.currentPhase2 + 
                                      this.boardData.voltagePhase3 * this.boardData.currentPhase3) / 1000;
        this.boardData.status = 'charging';
      } else {
        this.boardData.currentPhase1 = 0;
        this.boardData.currentPhase2 = 0;
        this.boardData.currentPhase3 = 0;
        this.boardData.chargerPower = 0;
        this.boardData.status = 'available';
      }

      // Имитируем случайные ошибки
      if (Math.random() < 0.1) {
        this.boardData.carError = !this.boardData.carError;
        if (this.boardData.carError) {
          console.log(`[Плата ${this.boardId}] Ошибка автомобиля!`);
        }
      }
    } else {
      // Master плата - имитируем изменение общей мощности
      this.boardData.currentPower = Math.random() * this.boardData.maxPower;
      
      // Случайно меняем статус
      const statuses = ['available', 'charging', 'maintenance'];
      if (Math.random() < 0.2) {
        this.boardData.status = statuses[Math.floor(Math.random() * statuses.length)];
      }
    }
  }

  /**
   * Отправка обновленных данных на сервер
   */
  async sendDataUpdate() {
    try {
      const response = await this.makeRequest('PATCH', `/api/stations/${this.boardId}`, this.boardData);
      
      if (response.success) {
        console.log(`[Плата ${this.boardId}] Данные обновлены: ${this.boardData.status}, мощность: ${this.boardData.currentPower?.toFixed(1) || 0}кВт`);
      }
    } catch (error) {
      console.error(`[Плата ${this.boardId}] Ошибка обновления данных:`, error.message);
    }
  }

  /**
   * Отключение от сервера
   */
  disconnect() {
    if (this.updateInterval) {
      clearInterval(this.updateInterval);
      this.updateInterval = null;
    }
    this.isConnected = false;
    console.log(`[Плата ${this.boardId}] Отключена от сервера`);
  }

  /**
   * Выполнение HTTP запроса к серверу
   */
  makeRequest(method, path, data = null) {
    return new Promise((resolve, reject) => {
      const options = {
        hostname: 'localhost',
        port: 5000,
        path: path,
        method: method,
        headers: {
          'Content-Type': 'application/json'
        }
      };

      const req = http.request(options, (res) => {
        let responseData = '';
        
        res.on('data', (chunk) => {
          responseData += chunk;
        });
        
        res.on('end', () => {
          try {
            const parsedData = responseData ? JSON.parse(responseData) : {};
            resolve({
              success: res.statusCode >= 200 && res.statusCode < 300,
              data: parsedData,
              statusCode: res.statusCode
            });
          } catch (error) {
            reject(new Error(`Ошибка парсинга ответа: ${error.message}`));
          }
        });
      });

      req.on('error', (error) => {
        reject(new Error(`Ошибка запроса: ${error.message}`));
      });

      if (data) {
        req.write(JSON.stringify(data));
      }
      
      req.end();
    });
  }
}

// Обработка аргументов командной строки
function parseArguments() {
  const args = process.argv.slice(2);
  const boardId = parseInt(args[0]) || 2;
  const type = args[1] || 'slave';
  
  return { boardId, type };
}

// Основная функция
async function main() {
  const { boardId, type } = parseArguments();
  
  console.log('='.repeat(50));
  console.log('   СИМУЛЯТОР ПЛАТЫ ЗАРЯДНОЙ СТАНЦИИ');
  console.log('='.repeat(50));
  console.log(`ID платы: ${boardId}`);
  console.log(`Тип платы: ${type}`);
  console.log(`Сервер: http://localhost:5000`);
  console.log('='.repeat(50));
  
  const simulator = new BoardSimulator(boardId, type);
  
  // Обработка сигналов завершения
  process.on('SIGINT', () => {
    console.log('\n[Система] Получен сигнал завершения...');
    simulator.disconnect();
    process.exit(0);
  });
  
  process.on('SIGTERM', () => {
    console.log('\n[Система] Получен сигнал завершения...');
    simulator.disconnect();
    process.exit(0);
  });
  
  // Подключаемся к серверу
  await simulator.connect();
  
  // Поддерживаем соединение
  if (simulator.isConnected) {
    console.log('\n[Система] Симулятор запущен. Нажмите Ctrl+C для остановки.\n');
  }
}

// Проверяем, что файл запущен напрямую
if (require.main === module) {
  main().catch(error => {
    console.error('Критическая ошибка:', error.message);
    process.exit(1);
  });
}

module.exports = BoardSimulator;