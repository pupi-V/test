#!/usr/bin/env python3
"""
Скрипт автоматической сборки и загрузки ESP32 проекта
Автоматизирует процесс сборки прошивки и загрузки файлов
"""

import os
import sys
import subprocess
import time
import argparse

def run_command(command, description):
    """Выполнение команды с отображением прогресса"""
    print(f"\n{'='*50}")
    print(f"🔄 {description}")
    print(f"{'='*50}")
    
    try:
        result = subprocess.run(command, shell=True, check=True, 
                              capture_output=True, text=True)
        print(f"✅ {description} - УСПЕШНО")
        if result.stdout:
            print("Вывод:")
            print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ {description} - ОШИБКА")
        print(f"Код ошибки: {e.returncode}")
        if e.stdout:
            print("Стандартный вывод:")
            print(e.stdout)
        if e.stderr:
            print("Ошибки:")
            print(e.stderr)
        return False

def check_platformio():
    """Проверка установки PlatformIO"""
    try:
        result = subprocess.run("pio --version", shell=True, 
                              capture_output=True, text=True)
        if result.returncode == 0:
            print(f"✅ PlatformIO установлен: {result.stdout.strip()}")
            return True
        else:
            print("❌ PlatformIO не найден")
            return False
    except:
        print("❌ PlatformIO не найден")
        return False

def main():
    parser = argparse.ArgumentParser(description='Сборка и загрузка ESP32 проекта')
    parser.add_argument('--port', '-p', help='COM порт ESP32 (например, COM3)')
    parser.add_argument('--no-filesystem', action='store_true', 
                       help='Пропустить загрузку файловой системы')
    parser.add_argument('--only-build', action='store_true', 
                       help='Только сборка без загрузки')
    parser.add_argument('--monitor', action='store_true', 
                       help='Открыть Serial Monitor после загрузки')
    parser.add_argument('--update-web', action='store_true',
                       help='Обновить веб-интерфейс из основного проекта')
    
    args = parser.parse_args()
    
    print("🔌 ESP32 Charging Station Management System")
    print("📋 Автоматическая сборка и загрузка\n")
    
    # Проверка PlatformIO
    if not check_platformio():
        print("\nУстановите PlatformIO:")
        print("pip install platformio")
        sys.exit(1)
    
    # Проверка директории проекта
    if not os.path.exists("platformio.ini"):
        print("❌ Файл platformio.ini не найден")
        print("Запустите скрипт из корневой папки ESP32 проекта")
        sys.exit(1)
    
    success = True
    
    # Этап 0: Обновление веб-интерфейса (если запрошено)
    if args.update_web:
        if run_command("python scripts/build_web_interface.py", "Обновление веб-интерфейса"):
            print("🌐 Веб-интерфейс обновлен из основного проекта")
        else:
            print("⚠️  Ошибка обновления веб-интерфейса, продолжаем с текущими файлами")
    
    # Этап 1: Очистка проекта
    if run_command("pio run --target clean", "Очистка проекта"):
        print("🧹 Проект очищен")
    
    # Этап 2: Сборка проекта
    if not run_command("pio run", "Сборка прошивки"):
        success = False
    
    # Этап 3: Сборка файловой системы
    if not args.no_filesystem:
        if not run_command("pio run --target buildfs", "Сборка файловой системы"):
            success = False
    
    if not success:
        print("\n❌ Ошибки при сборке проекта")
        sys.exit(1)
    
    if args.only_build:
        print("\n✅ Сборка завершена успешно")
        sys.exit(0)
    
    # Этап 4: Загрузка прошивки
    upload_cmd = "pio run --target upload"
    if args.port:
        upload_cmd += f" --upload-port {args.port}"
    
    print("\n📤 Начинаем загрузку прошивки...")
    print("💡 Если ESP32 не входит в режим программирования:")
    print("   1. Зажмите кнопку BOOT на плате")
    print("   2. Нажмите кнопку Reset (или отключите/подключите питание)")
    print("   3. Отпустите кнопку BOOT")
    
    input("\nНажмите Enter для продолжения...")
    
    if not run_command(upload_cmd, "Загрузка прошивки"):
        print("\n❌ Ошибка загрузки прошивки")
        print("Попробуйте:")
        print("1. Проверить подключение ESP32")
        print("2. Зажать кнопку BOOT при загрузке")
        print("3. Указать правильный COM порт: --port COM3")
        sys.exit(1)
    
    # Этап 5: Загрузка файловой системы
    if not args.no_filesystem:
        print("\n💾 Загрузка файлов веб-интерфейса...")
        
        uploadfs_cmd = "pio run --target uploadfs"
        if args.port:
            uploadfs_cmd += f" --upload-port {args.port}"
        
        if not run_command(uploadfs_cmd, "Загрузка файловой системы"):
            print("\n⚠️  Ошибка загрузки файловой системы")
            print("Система будет работать, но веб-интерфейс может быть недоступен")
    
    print("\n🎉 ЗАГРУЗКА ЗАВЕРШЕНА УСПЕШНО!")
    print("\n📋 Следующие шаги:")
    print("1. 🔌 Подключитесь к WiFi сети 'ESP32_ChargingStations'")
    print("2. 🌐 Откройте браузер и перейдите по адресу: http://192.168.4.1")
    print("3. 📱 Или используйте: http://chargingstations.local")
    
    # Запуск Serial Monitor
    if args.monitor:
        print("\n🖥️  Запуск Serial Monitor...")
        time.sleep(2)
        
        monitor_cmd = "pio device monitor"
        if args.port:
            monitor_cmd += f" --port {args.port}"
        
        try:
            subprocess.run(monitor_cmd, shell=True)
        except KeyboardInterrupt:
            print("\n👋 Serial Monitor остановлен")
    else:
        print("\n💡 Для просмотра логов выполните: pio device monitor")
        if args.port:
            print(f"   или: pio device monitor --port {args.port}")

if __name__ == "__main__":
    main()