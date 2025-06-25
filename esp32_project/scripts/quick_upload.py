
#!/usr/bin/env python3
"""
Упрощенный скрипт быстрой загрузки ESP32
Только самые необходимые шаги без избыточных проверок
"""

import os
import sys
import subprocess
import argparse

def main():
    parser = argparse.ArgumentParser(description='Быстрая загрузка ESP32')
    parser.add_argument('--port', '-p', help='COM порт ESP32')
    parser.add_argument('--skip-fs', action='store_true', help='Пропустить файловую систему')
    
    args = parser.parse_args()
    
    print("⚡ Быстрая загрузка ESP32")
    print("=" * 30)
    
    # Проверка директории проекта
    if not os.path.exists("platformio.ini"):
        print("❌ Запустите из корневой папки ESP32 проекта")
        sys.exit(1)
    
    # Команды для выполнения
    commands = [
        ("pio run", "Сборка прошивки"),
    ]
    
    if not args.skip_fs:
        commands.append(("pio run --target buildfs", "Сборка файловой системы"))
    
    # Выполнение команд сборки
    for cmd, desc in commands:
        print(f"\n🔄 {desc}...")
        result = subprocess.run(cmd, shell=True)
        if result.returncode != 0:
            print(f"⚠️  Предупреждение: {desc} завершилась с кодом {result.returncode}")
    
    # Загрузка прошивки
    upload_cmd = "pio run --target upload"
    if args.port:
        upload_cmd += f" --upload-port {args.port}"
    
    print(f"\n📤 Загрузка прошивки...")
    print("💡 Зажмите кнопку BOOT на ESP32 если необходимо")
    
    result = subprocess.run(upload_cmd, shell=True)
    if result.returncode == 0:
        print("✅ Прошивка загружена успешно!")
    else:
        print(f"❌ Ошибка загрузки прошивки (код: {result.returncode})")
        sys.exit(1)
    
    # Загрузка файловой системы
    if not args.skip_fs:
        uploadfs_cmd = "pio run --target uploadfs"
        if args.port:
            uploadfs_cmd += f" --upload-port {args.port}"
        
        print(f"\n💾 Загрузка файловой системы...")
        result = subprocess.run(uploadfs_cmd, shell=True)
        if result.returncode == 0:
            print("✅ Файловая система загружена успешно!")
        else:
            print(f"⚠️  Файловая система не загружена (код: {result.returncode})")
    
    print("\n🎉 Загрузка завершена!")
    print("📋 Подключитесь к WiFi: 'ESP32_ChargingStations'")
    print("🌐 Откройте: http://192.168.4.1")

if __name__ == "__main__":
    main()
