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



## User Preferences

Preferred communication style: Simple, everyday language.

## Recent Changes

### Очистка проекта от ненужных файлов (ЗАВЕРШЕНО)
- June 25, 2025: Удалены все остатки Node.js backend и устаревшие файлы
- Удалены папки: server/, shared/, esp32/
- Удалены документационные файлы: MIGRATION_COMPLETE.md, C_SERVER_SUCCESS.md, IMPLEMENTATION_SUMMARY.md
- Удалены скрипты запуска: start-simple.bat, start-windows.bat, run_full_app.sh и другие
- Удалены конфигурационные файлы: tsconfig.json, components.json, postcss.config.js
- Проект теперь содержит только необходимые файлы для работы

### Полное исправление системы обновления данных станций (ЗАВЕРШЕНО)
- June 25, 2025: Полностью решена критическая проблема с сохранением данных станций
- Реализована система глобального хранения данных в памяти C-сервера
- Исправлена логика селективного обновления полей (только переданные поля изменяются)
- Добавлен полнофункциональный API endpoint PATCH /api/stations/:id
- Исправлен HTTP парсер для корректной обработки тела запроса
- Реализован полноценный JSON парсер вместо заглушки
- Добавлена поддержка всех типов данных (строки, числа, булевые значения)
- Система корректно сохраняет и сохраняет изменения между запросами
- Все изменения мгновенно отражаются в веб-интерфейсе и остаются постоянными

### Реализация постоянного сохранения данных (ЗАВЕРШЕНО)
- June 25, 2025: Добавлена автоматическая синхронизация данных в файлы
- Функция save_global_stations_to_file() сохраняет все изменения в JSON файлы
- Автоматическое обновление data/stations.json при каждом изменении
- Синхронизация между основным файлом и локальной копией C-сервера
- Данные теперь сохраняются постоянно и не теряются при перезапуске

### Создание ESP32 версии системы (ЗАВЕРШЕНО)
- June 25, 2025: Адаптирована система для развертывания на ESP32 микроконтроллере
- Создан полнофункциональный C++ код для ESP32 с веб-сервером
- Упрощенный HTML/CSS/JS интерфейс оптимизированный для ограниченных ресурсов
- Поддержка SPIFFS для хранения веб-файлов и данных станций
- API совместимый с основной системой для управления зарядными станциями
- Подробная инструкция по установке и настройке ESP32
- Поддержка WiFi подключения и точки доступа
- Ограничение: максимум 10 станций, 4-8 одновременных подключений

### Создание ESP32 16MB полнофункциональной версии (ЗАВЕРШЕНО)
- June 25, 2025: Разработана расширенная версия для ESP32 с 16MB Flash памятью
- Полноценный React интерфейс с всеми оригинальными компонентами
- WebSocket поддержка для real-time обновлений каждые 5 секунд
- Поддержка до 50 зарядных станций и 20+ одновременных подключений
- LittleFS файловая система для эффективного хранения данных
- OTA обновления через веб-интерфейс и WiFi
- Система логирования и мониторинга производительности
- Автоматизированные скрипты сборки и развертывания
- Детальная инструкция по настройке и оптимизации
- Темная/светлая тема, модальные окна, валидация данных

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