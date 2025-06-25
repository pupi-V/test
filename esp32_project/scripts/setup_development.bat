@echo off
chcp 65001 >nul
echo.
echo ================================================
echo 🔧 Настройка среды разработки ESP32
echo ================================================
echo.

REM Проверка Python
echo 🐍 Проверка Python...
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ Python не найден
    echo 📥 Скачайте Python с https://python.org
    echo 💡 Убедитесь, что Python добавлен в PATH
    pause
    exit /b 1
) else (
    echo ✅ Python установлен
)

REM Проверка pip
echo.
echo 📦 Проверка pip...
pip --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ pip не найден
    pause
    exit /b 1
) else (
    echo ✅ pip доступен
)

REM Установка PlatformIO
echo.
echo 🔧 Установка PlatformIO...
pip install platformio
if %errorlevel% neq 0 (
    echo ❌ Ошибка установки PlatformIO
    pause
    exit /b 1
) else (
    echo ✅ PlatformIO установлен
)

REM Обновление PlatformIO
echo.
echo 🔄 Обновление платформ...
pio platform update

REM Установка платформы ESP32
echo.
echo 📲 Установка платформы ESP32...
pio platform install espressif32

REM Проверка установки
echo.
echo ✅ Проверка установки...
pio --version
pio platform list | findstr espressif32

echo.
echo ================================================
echo ✅ НАСТРОЙКА ЗАВЕРШЕНА УСПЕШНО!
echo ================================================
echo.
echo 📋 Следующие шаги:
echo 1. 📁 Откройте VS Code
echo 2. 🔌 Установите расширение "PlatformIO IDE"
echo 3. 📂 Откройте папку esp32_project в VS Code
echo 4. 🔨 Соберите проект: pio run
echo 5. 📤 Загрузите прошивку: pio run --target upload
echo.
echo 💡 Полная инструкция в файле README.md
echo.
pause