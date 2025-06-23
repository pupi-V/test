#!/usr/bin/env pwsh

Write-Host "Starting EV Charging Station Management System..." -ForegroundColor Green
Write-Host ""

# Check if Node.js is installed
try {
    $nodeVersion = node --version
    Write-Host "Node.js version: $nodeVersion" -ForegroundColor Blue
} catch {
    Write-Host "ERROR: Node.js is not installed or not in PATH" -ForegroundColor Red
    Write-Host "Please install Node.js from https://nodejs.org/" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Check if dependencies are installed
if (-not (Test-Path "node_modules")) {
    Write-Host "Installing dependencies..." -ForegroundColor Yellow
    npm install
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to install dependencies" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
}

# Set environment variables
$env:NODE_ENV = "development"
$env:PORT = "5000"
$env:HOST = "0.0.0.0"

Write-Host ""
Write-Host "Starting server..." -ForegroundColor Green
Write-Host "Access the application at:" -ForegroundColor Cyan
Write-Host "  - Local:    http://localhost:5000" -ForegroundColor White
Write-Host "  - Network:  http://YOUR_IP_ADDRESS:5000" -ForegroundColor White
Write-Host ""
Write-Host "Press Ctrl+C to stop the server" -ForegroundColor Yellow
Write-Host ""

# Start the server
node run-dev.js