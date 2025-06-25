@echo off
echo Загрузка файлов веб-интерфейса в SPIFFS...
echo.

REM Проверка наличия PlatformIO
pio --version >nul 2>&1
if errorlevel 1 (
    echo Ошибка: PlatformIO не найден. Установите PlatformIO CLI.
    pause
    exit /b 1
)

REM Загрузка файлов в SPIFFS
echo Загружаем файлы data/ в SPIFFS ESP32...
pio run --target uploadfs

if errorlevel 1 (
    echo.
    echo Ошибка загрузки файлов в SPIFFS!
    echo Проверьте подключение ESP32 и настройки COM порта.
) else (
    echo.
    echo Файлы успешно загружены в SPIFFS!
    echo Теперь можете загрузить основной код программы.
)

echo.
pause