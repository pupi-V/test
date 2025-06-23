@echo off
echo Starting EV Charging Station Management System...
echo.

REM Check if Node.js is installed
node --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Node.js is not installed or not in PATH
    echo Please install Node.js from https://nodejs.org/
    pause
    exit /b 1
)

REM Check if dependencies are installed
if not exist "node_modules" (
    echo Installing dependencies...
    npm install
    if errorlevel 1 (
        echo ERROR: Failed to install dependencies
        pause
        exit /b 1
    )
)

REM Set environment variables for Windows
set NODE_ENV=development
set PORT=5000
set HOST=0.0.0.0

echo.
echo Starting server...
echo Access the application at:
echo   - Local:    http://localhost:5000
echo   - Network:  http://YOUR_IP_ADDRESS:5000
echo.
echo Press Ctrl+C to stop the server
echo.

tsx server/index.ts