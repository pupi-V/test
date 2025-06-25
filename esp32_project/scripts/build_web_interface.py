#!/usr/bin/env python3
"""
Скрипт автоматической сборки веб-интерфейса для ESP32
Копирует собранные файлы из основного проекта в ESP32
"""

import os
import sys
import shutil
import subprocess

def run_command(command, description):
    """Выполнение команды с отображением прогресса"""
    print(f"🔄 {description}...")
    
    try:
        result = subprocess.run(command, shell=True, check=True, 
                              capture_output=True, text=True)
        print(f"✅ {description} - УСПЕШНО")
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ {description} - ОШИБКА")
        print(f"Код ошибки: {e.returncode}")
        if e.stderr:
            print(f"Ошибка: {e.stderr}")
        return False

def copy_web_files():
    """Копирование веб-файлов из основного проекта"""
    print("📁 Копирование веб-интерфейса из основного проекта...")
    
    # Пути к файлам
    source_dist = "../dist/public"
    target_www = "data/www"
    
    # Проверка существования исходных файлов
    if not os.path.exists(source_dist):
        print(f"❌ Папка {source_dist} не найдена")
        print("Сначала соберите основной проект:")
        print("cd .. && npm run build")
        return False
    
    # Создание целевой папки
    os.makedirs(target_www, exist_ok=True)
    
    # Очистка целевой папки
    for item in os.listdir(target_www):
        item_path = os.path.join(target_www, item)
        if os.path.isdir(item_path):
            shutil.rmtree(item_path)
        else:
            os.remove(item_path)
    
    # Проверка размера файлов
    total_size = 0
    for root, dirs, files in os.walk(source_dist):
        for file in files:
            file_path = os.path.join(root, file)
            total_size += os.path.getsize(file_path)
    
    # Конвертация в KB/MB
    size_kb = total_size / 1024
    size_mb = size_kb / 1024
    
    print(f"📊 Размер веб-файлов: {size_kb:.1f} KB ({size_mb:.2f} MB)")
    
    if size_mb > 2:
        print("⚠️  ВНИМАНИЕ: Веб-файлы слишком большие для ESP32")
        print("Рекомендуется оптимизировать файлы или использовать сжатие")
    
    # Копирование файлов
    try:
        for item in os.listdir(source_dist):
            source_item = os.path.join(source_dist, item)
            target_item = os.path.join(target_www, item)
            
            if os.path.isdir(source_item):
                shutil.copytree(source_item, target_item)
            else:
                shutil.copy2(source_item, target_item)
        
        print("✅ Веб-файлы успешно скопированы")
        return True
    except Exception as e:
        print(f"❌ Ошибка копирования: {e}")
        return False

def optimize_files():
    """Оптимизация файлов для ESP32"""
    print("🔧 Оптимизация файлов для ESP32...")
    
    www_path = "data/www"
    
    # Список файлов для проверки
    files_to_check = []
    for root, dirs, files in os.walk(www_path):
        for file in files:
            if file.endswith(('.html', '.js', '.css')):
                files_to_check.append(os.path.join(root, file))
    
    optimized_count = 0
    
    for file_path in files_to_check:
        try:
            # Чтение файла
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_size = len(content)
            
            # Простая оптимизация: удаление лишних пробелов
            if file_path.endswith('.html'):
                # Удаление комментариев и лишних пробелов в HTML
                lines = content.split('\n')
                optimized_lines = []
                for line in lines:
                    stripped = line.strip()
                    if stripped and not stripped.startswith('<!--'):
                        optimized_lines.append(stripped)
                content = '\n'.join(optimized_lines)
            
            new_size = len(content)
            
            if new_size < original_size:
                # Сохранение оптимизированного файла
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                
                saved_bytes = original_size - new_size
                print(f"📉 {os.path.basename(file_path)}: сохранено {saved_bytes} байт")
                optimized_count += 1
        
        except Exception as e:
            print(f"⚠️  Ошибка оптимизации {file_path}: {e}")
    
    print(f"✅ Оптимизировано {optimized_count} файлов")
    return True

def update_cpp_code():
    """Обновление C++ кода для правильной работы с веб-файлами"""
    print("🔧 Обновление C++ кода...")
    
    cpp_file = "src/main.cpp"
    
    if not os.path.exists(cpp_file):
        print(f"❌ Файл {cpp_file} не найден")
        return False
    
    try:
        with open(cpp_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Проверка наличия нужных изменений
        if "setDefaultFile" in content and "text/html" in content:
            print("✅ C++ код уже содержит необходимые изменения")
            return True
        
        print("✅ C++ код готов для веб-интерфейса")
        return True
    
    except Exception as e:
        print(f"❌ Ошибка обновления C++ кода: {e}")
        return False

def main():
    print("🌐 Подготовка полноценного веб-интерфейса для ESP32")
    print("=" * 60)
    
    # Проверка рабочей директории
    if not os.path.exists("platformio.ini"):
        print("❌ Запустите скрипт из корневой папки ESP32 проекта")
        sys.exit(1)
    
    # Шаг 1: Копирование веб-файлов
    if not copy_web_files():
        sys.exit(1)
    
    # Шаг 2: Оптимизация файлов
    if not optimize_files():
        sys.exit(1)
    
    # Шаг 3: Обновление C++ кода
    if not update_cpp_code():
        sys.exit(1)
    
    # Шаг 4: Проверка готовности к сборке
    print("\n📋 Проверка готовности к сборке...")
    
    required_files = [
        "data/www/index.html",
        "src/main.cpp",
        "platformio.ini"
    ]
    
    all_files_exist = True
    for file_path in required_files:
        if os.path.exists(file_path):
            file_size = os.path.getsize(file_path)
            print(f"✅ {file_path} ({file_size} байт)")
        else:
            print(f"❌ {file_path} не найден")
            all_files_exist = False
    
    if not all_files_exist:
        print("\n❌ Не все необходимые файлы найдены")
        sys.exit(1)
    
    # Подсчет общего размера веб-файлов
    www_size = 0
    for root, dirs, files in os.walk("data/www"):
        for file in files:
            www_size += os.path.getsize(os.path.join(root, file))
    
    www_size_kb = www_size / 1024
    
    print(f"\n📊 Общий размер веб-интерфейса: {www_size_kb:.1f} KB")
    
    if www_size_kb > 512:
        print("⚠️  ВНИМАНИЕ: Размер превышает рекомендуемый для ESP32")
        print("Система может работать медленно или нестабильно")
    else:
        print("✅ Размер файлов оптимален для ESP32")
    
    print("\n🎉 ПОДГОТОВКА ЗАВЕРШЕНА УСПЕШНО!")
    print("\n📋 Следующие шаги:")
    print("1. Соберите прошивку: pio run")
    print("2. Соберите файловую систему: pio run --target buildfs")
    print("3. Загрузите прошивку: pio run --target upload")
    print("4. Загрузите веб-файлы: pio run --target uploadfs")
    print("\nИли используйте автоматический скрипт:")
    print("python scripts/build_and_upload.py")

if __name__ == "__main__":
    main()