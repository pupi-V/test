#!/usr/bin/env node

/**
 * Скрипт для загрузки веб-интерфейса в ESP32 через WiFi
 * Использует HTTP API для загрузки файлов в LittleFS
 */

const fs = require('fs-extra');
const path = require('path');
const axios = require('axios');
const FormData = require('form-data');

// Конфигурация
const ESP32_IP = process.env.ESP32_IP || '192.168.1.100';
const ESP32_PORT = process.env.ESP32_PORT || '80';
const ESP32_URL = `http://${ESP32_IP}:${ESP32_PORT}`;
const WEB_DIR = './data/dist';

console.log('📡 Загрузка веб-интерфейса в ESP32 16MB...');
console.log(`🎯 Целевой ESP32: ${ESP32_URL}\n`);

async function uploadWebInterface() {
  try {
    // 1. Проверка подключения к ESP32
    console.log('1️⃣ Проверка подключения к ESP32...');
    await checkESP32Connection();

    // 2. Получение списка файлов для загрузки
    console.log('2️⃣ Сканирование файлов для загрузки...');
    const files = await getFilesRecursively(WEB_DIR);
    console.log(`   Найдено ${files.length} файлов`);

    // 3. Очистка существующих файлов (опционально)
    if (process.argv.includes('--clean')) {
      console.log('3️⃣ Очистка существующих файлов...');
      await cleanExistingFiles();
    }

    // 4. Загрузка файлов
    console.log('4️⃣ Загрузка файлов...');
    await uploadFiles(files);

    // 5. Проверка загрузки
    console.log('5️⃣ Проверка загрузки...');
    await verifyUpload();

    console.log('\n✅ Загрузка завершена успешно!');
    console.log(`🌐 Веб-интерфейс доступен: ${ESP32_URL}`);

  } catch (error) {
    console.error('\n❌ Ошибка загрузки:', error.message);
    process.exit(1);
  }
}

async function checkESP32Connection() {
  try {
    const response = await axios.get(`${ESP32_URL}/api/system/info`, {
      timeout: 5000
    });
    
    console.log(`   ✅ ESP32 доступен`);
    console.log(`   📊 Модель: ${response.data.chipModel}`);
    console.log(`   💾 Flash: ${(response.data.flashSize / 1024 / 1024).toFixed(1)}MB`);
    console.log(`   🆓 Свободная память: ${(response.data.freeHeap / 1024).toFixed(1)}KB`);
    
  } catch (error) {
    throw new Error(`Не удается подключиться к ESP32 по адресу ${ESP32_URL}`);
  }
}

async function getFilesRecursively(dir) {
  const files = [];
  
  async function scan(currentDir, relativePath = '') {
    const items = await fs.readdir(currentDir);
    
    for (const item of items) {
      const fullPath = path.join(currentDir, item);
      const itemRelativePath = relativePath ? `${relativePath}/${item}` : item;
      const stat = await fs.stat(fullPath);
      
      if (stat.isDirectory()) {
        await scan(fullPath, itemRelativePath);
      } else {
        files.push({
          localPath: fullPath,
          remotePath: `/${itemRelativePath}`,
          size: stat.size
        });
      }
    }
  }
  
  await scan(dir);
  return files;
}

async function cleanExistingFiles() {
  try {
    await axios.delete(`${ESP32_URL}/api/files`, {
      timeout: 10000
    });
    console.log('   ✅ Существующие файлы очищены');
  } catch (error) {
    console.log('   ⚠️  Не удалось очистить файлы (возможно, endpoint не поддерживается)');
  }
}

async function uploadFiles(files) {
  const totalSize = files.reduce((sum, file) => sum + file.size, 0);
  let uploadedSize = 0;
  
  console.log(`   📦 Общий размер: ${(totalSize / 1024 / 1024).toFixed(2)}MB`);
  
  for (let i = 0; i < files.length; i++) {
    const file = files[i];
    
    try {
      await uploadSingleFile(file);
      uploadedSize += file.size;
      
      const progress = ((i + 1) / files.length * 100).toFixed(1);
      const sizeProgress = (uploadedSize / totalSize * 100).toFixed(1);
      
      console.log(`   📤 [${progress}%] ${file.remotePath} (${(file.size / 1024).toFixed(1)}KB)`);
      
    } catch (error) {
      console.error(`   ❌ Ошибка загрузки ${file.remotePath}: ${error.message}`);
      
      // Пытаемся продолжить с другими файлами
      if (!process.argv.includes('--continue-on-error')) {
        throw error;
      }
    }
  }
}

async function uploadSingleFile(file) {
  const formData = new FormData();
  const fileStream = fs.createReadStream(file.localPath);
  
  formData.append('file', fileStream, {
    filename: path.basename(file.remotePath),
    contentType: getContentType(file.remotePath)
  });
  formData.append('path', file.remotePath);
  
  await axios.post(`${ESP32_URL}/api/files/upload`, formData, {
    headers: {
      ...formData.getHeaders(),
    },
    timeout: 30000,
    maxContentLength: 10 * 1024 * 1024, // 10MB лимит
  });
}

function getContentType(filename) {
  const ext = path.extname(filename).toLowerCase();
  const mimeTypes = {
    '.html': 'text/html',
    '.css': 'text/css',
    '.js': 'application/javascript',
    '.json': 'application/json',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.gif': 'image/gif',
    '.svg': 'image/svg+xml',
    '.ico': 'image/x-icon',
    '.woff': 'font/woff',
    '.woff2': 'font/woff2',
    '.ttf': 'font/ttf',
    '.eot': 'application/vnd.ms-fontobject'
  };
  
  return mimeTypes[ext] || 'application/octet-stream';
}

async function verifyUpload() {
  try {
    // Проверяем, что главная страница доступна
    const response = await axios.get(ESP32_URL, {
      timeout: 10000,
      headers: {
        'Accept': 'text/html'
      }
    });
    
    if (response.status === 200 && response.data.includes('Charging Station')) {
      console.log('   ✅ Главная страница загружается корректно');
    } else {
      console.log('   ⚠️  Главная страница загружается, но содержимое может быть неполным');
    }
    
    // Проверяем API
    const apiResponse = await axios.get(`${ESP32_URL}/api/stations`, {
      timeout: 5000
    });
    
    if (apiResponse.status === 200) {
      console.log('   ✅ API доступно и работает');
    }
    
  } catch (error) {
    console.log('   ⚠️  Не удалось полностью проверить загрузку');
  }
}

// Обработка аргументов командной строки
if (process.argv.includes('--help') || process.argv.includes('-h')) {
  console.log(`
Использование: node upload_to_esp32.js [опции]

Опции:
  --clean                    Очистить существующие файлы перед загрузкой
  --continue-on-error        Продолжить загрузку при ошибках отдельных файлов
  --esp32-ip=192.168.1.100   IP адрес ESP32 (по умолчанию: 192.168.1.100)
  --help, -h                 Показать эту справку

Переменные окружения:
  ESP32_IP                   IP адрес ESP32
  ESP32_PORT                 Порт ESP32 (по умолчанию: 80)

Примеры:
  node upload_to_esp32.js --esp32-ip=192.168.1.150
  ESP32_IP=192.168.1.200 node upload_to_esp32.js --clean
`);
  process.exit(0);
}

// Парсинг IP из аргументов
const ipArg = process.argv.find(arg => arg.startsWith('--esp32-ip='));
if (ipArg) {
  const ip = ipArg.split('=')[1];
  if (ip) {
    process.env.ESP32_IP = ip;
  }
}

// Запуск загрузки
uploadWebInterface();