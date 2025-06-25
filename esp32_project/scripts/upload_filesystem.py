#!/usr/bin/env python3
"""
Скрипт загрузки файловой системы в ESP32
Автоматизирует процесс загрузки веб-файлов в LittleFS
"""

import os
import subprocess
import sys
import argparse

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

def main():
    parser = argparse.ArgumentParser(description='Загрузка файловой системы ESP32')
    parser.add_argument('--port', '-p', help='COM порт ESP32')
    parser.add_argument('--build-only', action='store_true', 
                       help='Только сборка без загрузки')
    
    args = parser.parse_args()
    
    print("💾 ESP32 Загрузка файловой системы")
    print("=" * 40)
    
    # Проверка наличия файлов
    required_files = [
        "data/www/index.html",
        "data/stations.json"
    ]
    
    for file_path in required_files:
        if not os.path.exists(file_path):
            print(f"❌ Файл не найден: {file_path}")
            sys.exit(1)
    
    print("✅ Все необходимые файлы найдены")
    
    # Сборка файловой системы
    if not run_command("pio run --target buildfs", "Сборка файловой системы"):
        sys.exit(1)
    
    if args.build_only:
        print("✅ Сборка файловой системы завершена")
        sys.exit(0)
    
    # Загрузка файловой системы
    upload_cmd = "pio run --target uploadfs"
    if args.port:
        upload_cmd += f" --upload-port {args.port}"
    
    print("\n💡 Убедитесь, что ESP32 подключен и готов к загрузке")
    input("Нажмите Enter для продолжения...")
    
    if run_command(upload_cmd, "Загрузка файловой системы"):
        print("\n🎉 Файловая система успешно загружена!")
        print("📋 Файлы в ESP32:")
        print("  - /www/index.html - веб-интерфейс")
        print("  - /stations.json - данные станций")
        print("\nПерезагрузите ESP32 для применения изменений")
    else:
        print("\n❌ Ошибка загрузки файловой системы")
        sys.exit(1)

if __name__ == "__main__":
    main()