import { exec } from 'child_process';
import { promisify } from 'util';

const execAsync = promisify(exec);

interface ESP32BoardInfo {
  id: string;
  type: 'master' | 'slave';
  ip: string;
  name: string;
  technicalName: string;
  maxPower: number;
  status: 'online' | 'offline';
  lastSeen: string;
}

export async function scanForESP32Boards(): Promise<ESP32BoardInfo[]> {
  const foundBoards: ESP32BoardInfo[] = [];
  
  try {
    const networkInfo = await getLocalNetworkInfo();
    const scanPromises = [];
    
    for (let i = 1; i <= 254; i++) {
      const ip = `${networkInfo.baseIp}.${i}`;
      scanPromises.push(scanSingleIP(ip));
    }
    
    const results = await Promise.allSettled(scanPromises);
    
    for (const result of results) {
      if (result.status === 'fulfilled' && result.value) {
        foundBoards.push(result.value);
      }
    }
    
    console.log(`Сканирование завершено. Найдено ESP32 плат: ${foundBoards.length}`);
    return foundBoards;
    
  } catch (error) {
    console.error('Ошибка при сканировании сети:', error);
    return [];
  }
}

/**
 * Получение информации о локальной сети
 */
async function getLocalNetworkInfo() {
  try {
    // Получаем IP адрес текущей машины
    const { stdout } = await execAsync('hostname -I');
    const localIp = stdout.trim().split(' ')[0];
    
    // Извлекаем базовый IP (первые 3 октета)
    const ipParts = localIp.split('.');
    const baseIp = `${ipParts[0]}.${ipParts[1]}.${ipParts[2]}`;
    
    return { baseIp, localIp };
  } catch (error) {
    console.warn('Не удалось определить локальную сеть, используем 192.168.1');
    return { baseIp: '192.168.1', localIp: '192.168.1.1' };
  }
}

/**
 * Сканирование одного IP адреса для поиска ESP32
 */
async function scanSingleIP(ip: string): Promise<ESP32BoardInfo | null> {
  try {
    // Проверяем доступность устройства через ping
    const isReachable = await pingHost(ip);
    if (!isReachable) {
      return null;
    }
    
    // Проверяем, является ли устройство ESP32 платой зарядной станции
    const boardInfo = await checkIfESP32ChargingBoard(ip);
    
    return boardInfo;
  } catch (error) {
    return null;
  }
}

/**
 * Проверка доступности хоста через ping
 */
async function pingHost(ip: string): Promise<boolean> {
  try {
    await execAsync(`ping -c 1 -W 1 ${ip}`, { timeout: 2000 });
    return true;
  } catch (error) {
    return false;
  }
}

/**
 * Проверка, является ли устройство ESP32 платой зарядной станции
 */
async function checkIfESP32ChargingBoard(ip: string): Promise<ESP32BoardInfo | null> {
  try {
    // Стандартные порты ESP32 для HTTP (80, 8080)
    const ports = [80, 8080];
    
    for (const port of ports) {
      try {
        const response = await fetch(`http://${ip}:${port}/api/info`, {
          method: 'GET',
          headers: {
            'Content-Type': 'application/json',
          },
          signal: AbortSignal.timeout(3000)
        });
        
        if (response.ok) {
          const data = await response.json();
          
          // Проверяем, что это действительно плата зарядной станции
          if (data.device_type === 'charging_station' && 
              (data.board_type === 'master' || data.board_type === 'slave')) {
            
            return {
              id: data.board_id || `esp32_${ip.replace(/\./g, '_')}`,
              type: data.board_type,
              ip: ip,
              name: data.display_name || `ESP32 ${data.board_type}`,
              technicalName: data.technical_name || `esp32-${data.board_type}-${Date.now()}`,
              maxPower: data.max_power || (data.board_type === 'master' ? 250 : 22),
              status: 'online',
              lastSeen: new Date().toISOString()
            };
          }
        }
      } catch (error) {
        // Продолжаем проверку других портов
        continue;
      }
    }
    
    return null;
  } catch (error) {
    return null;
  }
}

/**
 * Подключение к конкретной плате ESP32 и получение её конфигурации
 */
export async function connectToESP32Board(ip: string, expectedType?: 'master' | 'slave'): Promise<ESP32BoardInfo | null> {
  try {
    // Проверяем доступность платы
    const isReachable = await pingHost(ip);
    if (!isReachable) {
      console.log(`Плата ${ip} недоступна по ping`);
      return null;
    }
    
    // Пытаемся подключиться к плате
    const ports = [80, 8080];
    
    for (const port of ports) {
      try {
        // Получаем информацию о плате
        const infoResponse = await fetch(`http://${ip}:${port}/api/info`, {
          method: 'GET',
          headers: {
            'Content-Type': 'application/json',
          },
          signal: AbortSignal.timeout(5000)
        });
        
        if (infoResponse.ok) {
          const boardData = await infoResponse.json();
          
          // Проверяем, что это плата зарядной станции
          if (boardData.device_type !== 'charging_station') {
            continue;
          }
          
          // Если ожидаемый тип указан, проверяем соответствие
          if (expectedType && boardData.board_type !== expectedType) {
            console.log(`Плата ${ip} имеет тип ${boardData.board_type}, ожидался ${expectedType}`);
            continue;
          }
          
          // Устанавливаем соединение с платой
          const connectResponse = await fetch(`http://${ip}:${port}/api/connect`, {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
            },
            body: JSON.stringify({
              client_type: 'web_interface',
              timestamp: Date.now()
            }),
            signal: AbortSignal.timeout(5000)
          });
          
          if (connectResponse.ok) {
            const connectData = await connectResponse.json();
            
            console.log(`Успешно подключились к плате ${ip} (${boardData.board_type})`);
            
            return {
              id: boardData.board_id || `esp32_${ip.replace(/\./g, '_')}`,
              type: boardData.board_type,
              ip: ip,
              name: boardData.display_name || `ESP32 ${boardData.board_type}`,
              technicalName: boardData.technical_name || `esp32-${boardData.board_type}-${Date.now()}`,
              maxPower: boardData.max_power || (boardData.board_type === 'master' ? 250 : 22),
              status: 'online',
              lastSeen: new Date().toISOString()
            };
          }
        }
      } catch (error) {
        // Продолжаем проверку других портов
        continue;
      }
    }
    
    console.log(`Не удалось подключиться к плате ${ip}`);
    return null;
    
  } catch (error) {
    console.error(`Ошибка при подключении к плате ${ip}:`, error);
    return null;
  }
}

/**
 * Отправка данных на плату ESP32
 */
export async function sendDataToESP32(ip: string, data: any): Promise<boolean> {
  try {
    const ports = [80, 8080];
    
    for (const port of ports) {
      try {
        const response = await fetch(`http://${ip}:${port}/api/data`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify(data),
          signal: AbortSignal.timeout(5000)
        });
        
        if (response.ok) {
          console.log(`Данные успешно отправлены на плату ${ip}`);
          return true;
        }
      } catch (error) {
        continue;
      }
    }
    
    return false;
  } catch (error) {
    console.error(`Ошибка отправки данных на плату ${ip}:`, error);
    return false;
  }
}

/**
 * Получение данных с платы ESP32
 */
export async function getDataFromESP32(ip: string): Promise<any | null> {
  try {
    const ports = [80, 8080];
    
    for (const port of ports) {
      try {
        const response = await fetch(`http://${ip}:${port}/api/data`, {
          method: 'GET',
          headers: {
            'Content-Type': 'application/json',
          },
          signal: AbortSignal.timeout(5000)
        });
        
        if (response.ok) {
          const data = await response.json();
          console.log(`Данные получены с платы ${ip}`);
          return data;
        }
      } catch (error) {
        continue;
      }
    }
    
    return null;
  } catch (error) {
    console.error(`Ошибка получения данных с платы ${ip}:`, error);
    return null;
  }
}