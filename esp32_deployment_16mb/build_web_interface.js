#!/usr/bin/env node

/**
 * Скрипт для сборки React интерфейса для ESP32 16MB
 * Оптимизирует размер и структуру для встроенной системы
 */

const fs = require('fs-extra');
const path = require('path');
const { execSync } = require('child_process');

const sourceDir = '../client';
const buildDir = './data/dist';
const tempDir = './temp_build';

console.log('🔧 Начинаем сборку веб-интерфейса для ESP32 16MB...\n');

async function buildWebInterface() {
  try {
    // 1. Очистка предыдущих сборок
    console.log('1️⃣ Очистка директорий...');
    await fs.remove(buildDir);
    await fs.remove(tempDir);
    await fs.ensureDir(buildDir);
    await fs.ensureDir(tempDir);

    // 2. Копирование исходников во временную директорию
    console.log('2️⃣ Копирование исходников...');
    await fs.copy(sourceDir, tempDir);

    // 3. Модификация конфигурации для ESP32
    console.log('3️⃣ Настройка конфигурации для ESP32...');
    await modifyConfigForESP32();

    // 4. Установка зависимостей
    console.log('4️⃣ Установка зависимостей...');
    execSync('npm install', { cwd: tempDir, stdio: 'inherit' });

    // 5. Сборка оптимизированной версии
    console.log('5️⃣ Сборка оптимизированной версии...');
    execSync('npm run build', { cwd: tempDir, stdio: 'inherit' });

    // 6. Копирование результата
    console.log('6️⃣ Копирование результатов...');
    const distPath = path.join(tempDir, 'dist');
    await fs.copy(distPath, buildDir);

    // 7. Оптимизация для ESP32
    console.log('7️⃣ Оптимизация для ESP32...');
    await optimizeForESP32();

    // 8. Создание манифеста файлов
    console.log('8️⃣ Создание манифеста файлов...');
    await createFileManifest();

    // 9. Очистка временных файлов
    console.log('9️⃣ Очистка временных файлов...');
    await fs.remove(tempDir);

    console.log('\n✅ Сборка завершена успешно!');
    console.log(`📁 Результат: ${buildDir}`);
    
    // Показываем статистику
    await showBuildStats();

  } catch (error) {
    console.error('\n❌ Ошибка сборки:', error.message);
    process.exit(1);
  }
}

async function modifyConfigForESP32() {
  // Модификация vite.config.ts для ESP32
  const viteConfigPath = path.join(tempDir, 'vite.config.ts');
  const viteConfig = `
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import path from 'path'

export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "./src"),
    },
  },
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    minify: 'esbuild',
    sourcemap: false,
    target: 'es2015',
    rollupOptions: {
      output: {
        manualChunks: {
          vendor: ['react', 'react-dom'],
          ui: ['@radix-ui/react-dialog', '@radix-ui/react-dropdown-menu'],
          utils: ['clsx', 'tailwind-merge']
        },
        assetFileNames: (assetInfo) => {
          const info = assetInfo.name.split('.');
          const ext = info[info.length - 1];
          if (/png|jpe?g|svg|gif|tiff|bmp|ico/i.test(ext)) {
            return \`assets/images/[name]-[hash].\${ext}\`;
          }
          if (/css/i.test(ext)) {
            return \`assets/css/[name]-[hash].\${ext}\`;
          }
          return \`assets/[name]-[hash].\${ext}\`;
        },
        chunkFileNames: 'assets/js/[name]-[hash].js',
        entryFileNames: 'assets/js/[name]-[hash].js',
      },
    },
    chunkSizeWarningLimit: 1000,
    reportCompressedSize: false,
  },
  base: '/',
  server: {
    host: '0.0.0.0',
    port: 5173,
  },
  preview: {
    host: '0.0.0.0',
    port: 4173,
  }
})
`;

  await fs.writeFile(viteConfigPath, viteConfig);

  // Модификация package.json для оптимизации
  const packageJsonPath = path.join(tempDir, 'package.json');
  const packageJson = await fs.readJson(packageJsonPath);
  
  packageJson.scripts.build = 'vite build --mode production';
  packageJson.scripts['build:esp32'] = 'vite build --mode production && node ../optimize-for-esp32.js';
  
  await fs.writeJson(packageJsonPath, packageJson, { spaces: 2 });
}

async function optimizeForESP32() {
  // Сжатие HTML файлов
  const htmlFiles = await getFilesByExtension(buildDir, '.html');
  for (const file of htmlFiles) {
    let content = await fs.readFile(file, 'utf8');
    
    // Минификация HTML
    content = content
      .replace(/>\s+</g, '><')
      .replace(/\s+/g, ' ')
      .trim();
    
    // Добавление meta тегов для ESP32
    content = content.replace('<head>', `<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <meta name="mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="theme-color" content="#667eea">`);
    
    await fs.writeFile(file, content);
  }

  // Оптимизация CSS файлов
  const cssFiles = await getFilesByExtension(buildDir, '.css');
  for (const file of cssFiles) {
    let content = await fs.readFile(file, 'utf8');
    
    // Удаление комментариев и лишних пробелов
    content = content
      .replace(/\/\*[\s\S]*?\*\//g, '')
      .replace(/\s+/g, ' ')
      .replace(/;\s*}/g, '}')
      .replace(/{\s*/g, '{')
      .replace(/;\s*/g, ';')
      .trim();
    
    await fs.writeFile(file, content);
  }

  // Оптимизация JS файлов (дополнительная минификация)
  const jsFiles = await getFilesByExtension(buildDir, '.js');
  for (const file of jsFiles) {
    let content = await fs.readFile(file, 'utf8');
    
    // Удаление console.log в production
    content = content.replace(/console\.log\([^)]*\);?/g, '');
    
    await fs.writeFile(file, content);
  }
}

async function getFilesByExtension(dir, ext) {
  const files = [];
  
  async function scan(currentDir) {
    const items = await fs.readdir(currentDir);
    
    for (const item of items) {
      const fullPath = path.join(currentDir, item);
      const stat = await fs.stat(fullPath);
      
      if (stat.isDirectory()) {
        await scan(fullPath);
      } else if (path.extname(item) === ext) {
        files.push(fullPath);
      }
    }
  }
  
  await scan(dir);
  return files;
}

async function createFileManifest() {
  const manifest = {
    version: "1.0.0",
    buildTime: new Date().toISOString(),
    files: {},
    totalSize: 0
  };

  async function processDir(dir, relativePath = '') {
    const items = await fs.readdir(dir);
    
    for (const item of items) {
      const fullPath = path.join(dir, item);
      const itemRelativePath = relativePath ? `${relativePath}/${item}` : item;
      const stat = await fs.stat(fullPath);
      
      if (stat.isDirectory()) {
        await processDir(fullPath, itemRelativePath);
      } else {
        const size = stat.size;
        manifest.files[itemRelativePath] = {
          size: size,
          hash: require('crypto').createHash('md5').update(await fs.readFile(fullPath)).digest('hex').substring(0, 8)
        };
        manifest.totalSize += size;
      }
    }
  }

  await processDir(buildDir);
  
  await fs.writeJson(path.join(buildDir, 'manifest.json'), manifest, { spaces: 2 });
}

async function showBuildStats() {
  const manifestPath = path.join(buildDir, 'manifest.json');
  
  if (await fs.pathExists(manifestPath)) {
    const manifest = await fs.readJson(manifestPath);
    
    console.log('\n📊 Статистика сборки:');
    console.log(`📁 Общий размер: ${(manifest.totalSize / 1024 / 1024).toFixed(2)} MB`);
    console.log(`📄 Количество файлов: ${Object.keys(manifest.files).length}`);
    console.log(`🏗️ Время сборки: ${manifest.buildTime}`);
    
    // Разбивка по типам файлов
    const fileTypes = {};
    Object.entries(manifest.files).forEach(([filename, info]) => {
      const ext = path.extname(filename).toLowerCase() || 'no-ext';
      if (!fileTypes[ext]) {
        fileTypes[ext] = { count: 0, size: 0 };
      }
      fileTypes[ext].count++;
      fileTypes[ext].size += info.size;
    });
    
    console.log('\n📈 Разбивка по типам файлов:');
    Object.entries(fileTypes)
      .sort(([,a], [,b]) => b.size - a.size)
      .forEach(([ext, info]) => {
        console.log(`  ${ext.padEnd(8)} ${info.count.toString().padStart(3)} файлов, ${(info.size / 1024).toFixed(1).padStart(8)} KB`);
      });
    
    // Проверка лимитов ESP32
    if (manifest.totalSize > 8 * 1024 * 1024) {
      console.log('\n⚠️  ВНИМАНИЕ: Размер превышает рекомендуемый лимит 8MB для LittleFS');
    } else {
      console.log('\n✅ Размер в пределах лимитов ESP32 16MB');
    }
  }
}

// Запуск сборки
buildWebInterface();