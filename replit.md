# EV Charging Station Management System

## Overview

This is a full-stack web application for managing and monitoring electric vehicle charging stations with ESP32 integration. The system provides both web-based management interfaces and embedded device support for distributed charging station networks.

## System Architecture

### Frontend Architecture
- **Framework**: React 18 with TypeScript
- **Styling**: Tailwind CSS with custom design system
- **UI Components**: Radix UI primitives with shadcn/ui components
- **State Management**: TanStack Query for server state management
- **Routing**: Wouter for client-side routing
- **Build Tool**: Vite for development and production builds

### Backend Architecture
- **Runtime**: C with custom HTTP server
- **Language**: C99 standard with GCC compiler
- **Storage**: JSON file-based data persistence
- **API Design**: RESTful API with JSON responses
- **Dependencies**: Only standard C libraries (no external dependencies)

### Key Components

#### Station Management
The core entity is the charging station with comprehensive monitoring capabilities:
- Station identification (display name, technical name, type)
- Power management (max power, current power, available power)
- Status tracking (charging, available, offline, maintenance)
- Connection monitoring (car connection, charging permissions, errors)
- Master-slave relationships for distributed control
- Real-time electrical parameters (voltage, current per phase)

#### User Interface Modes
1. **Board Selector**: Network discovery and station selection interface
2. **Master Control**: Central management dashboard for all stations
3. **Slave Control**: Individual station monitoring and control interface

#### ESP32 Integration
- Standalone web interface for embedded devices
- Network scanning and automatic discovery
- mDNS support for local network access
- Real-time data synchronization with main system
- Arduino IDE and PlatformIO support

## Data Flow

1. **Station Discovery**: ESP32 boards broadcast their presence on the local network
2. **Data Collection**: Real-time electrical parameters are collected from charging hardware
3. **State Management**: Station status is updated based on car connection, master permissions, and system health
4. **Web Interface**: React frontend polls the API every 5 seconds for live updates
5. **Control Commands**: User actions trigger API calls to update station parameters
6. **Persistence**: All changes are stored in PostgreSQL database with JSON fallback

## External Dependencies

### Production Dependencies
- **Database**: PostgreSQL (via Neon serverless)
- **UI Framework**: React ecosystem (Radix UI, Tailwind CSS)
- **Validation**: Zod for schema validation
- **HTTP Client**: Native fetch API with TanStack Query
- **Form Handling**: React Hook Form with Zod resolvers

### Development Dependencies
- **Build Tools**: Vite, esbuild for bundling
- **TypeScript**: Full type safety across frontend and backend
- **Database Tools**: Drizzle Kit for migrations
- **Development Server**: tsx for TypeScript execution

### ESP32 Dependencies
- **WiFi Connectivity**: ESP32 built-in WiFi
- **Web Server**: ESP32 WebServer library
- **JSON Processing**: ArduinoJson library
- **Network Discovery**: mDNS for local network broadcasting

## Deployment Strategy

### Development Environment
- **Startup Scripts**: Multiple Windows batch files and PowerShell scripts for easy local development
- **Cross-platform**: Node.js scripts work on Windows, macOS, and Linux
- **Hot Reload**: Vite development server with React Fast Refresh
- **Environment Variables**: .env file support for configuration

### Production Deployment
- **Platform**: Replit with autoscale deployment target
- **Build Process**: Vite builds frontend assets, esbuild bundles server
- **Database**: Automatic PostgreSQL provisioning via Drizzle configuration
- **Port Configuration**: Configurable host and port settings (default: 0.0.0.0:5000)

### ESP32 Deployment
- **Programming**: Arduino IDE or PlatformIO
- **WiFi Configuration**: Hard-coded SSID/password in firmware
- **Network Integration**: Automatic discovery by main system
- **Local Access**: Direct IP access or mDNS name resolution

## User Preferences

Preferred communication style: Simple, everyday language.

## Recent Changes

### Исправление проблем с обновлением данных станций (ЗАВЕРШЕНО)
- June 25, 2025: Полностью исправлена система обновления данных станций
- Добавлен полнофункциональный API endpoint PATCH /api/stations/:id
- Исправлен HTTP парсер для корректной обработки тела запроса
- Реализован полноценный JSON парсер вместо заглушки
- Добавлена поддержка всех типов данных (строки, числа, булевые значения)
- Система корректно сохраняет введенные пользователем данные в JSON файл
- Все изменения мгновенно отражаются в веб-интерфейсе

### Полный переход на C-сервер (ЗАВЕРШЕНО)
- June 24, 2025: Node.js backend полностью удален, остался только C-сервер
- Создан высокопроизводительный C-сервер с нуля
- Реализованы все API endpoints (/api/stations, /api/esp32/scan)
- Собственные HTTP и JSON библиотеки без внешних зависимостей
- Полная совместимость с React фронтендом сохранена
- Система сборки и запуска упрощена
- Минимальное потребление ресурсов

## Changelog

- June 24, 2025: Complete migration from Node.js to C backend
  - Deleted Node.js server directory and dependencies
  - C-server now serves as the only backend
  - Updated build scripts and documentation
  - System fully operational with C backend only