# Charging Station Management System

## Overview

This is a comprehensive electric vehicle charging station management system that consists of:
- A web application for centralized monitoring and control
- ESP32 firmware for physical hardware control
- Testing tools and simulators for development

The system enables real-time monitoring, configuration, and management of multiple charging stations through both a centralized web interface and standalone ESP32 devices.

## System Architecture

### Frontend Architecture
- **Framework**: React 18.3.1 with TypeScript
- **UI Library**: Radix UI components with Tailwind CSS styling
- **State Management**: TanStack Query for server state management
- **Routing**: Wouter for client-side routing
- **Form Handling**: React Hook Form with Zod validation
- **Theme System**: Light/dark theme support with system preference detection

### Backend Architecture
- **Runtime**: Node.js with Express.js server
- **Language**: TypeScript with ESM modules
- **API Design**: RESTful JSON API
- **Data Validation**: Zod schemas shared between client and server
- **Development**: Vite for build tooling and hot module replacement

### Data Storage Solutions
- **Database**: PostgreSQL with Drizzle ORM
- **Schema Management**: Drizzle Kit for migrations
- **Connection**: Neon Database serverless connection
- **Fallback**: JSON file storage for development/testing

## Key Components

### Web Application Components
1. **Master Control** (`/master`) - Main overview with station cards and filtering (renamed from dashboard)
2. **Board Selector** (`/board-selector`) - ESP32 device discovery and connection
3. **Slave Control** (`/slave/:id`) - Detailed control interface for slave stations
4. **Station Management** - CRUD operations for charging stations

### ESP32 Integration
- Network scanning for automatic device discovery
- HTTP client for communication with ESP32 boards
- Master/Slave architecture support
- Real-time data synchronization

### UI Component System
- Consistent design system using shadcn/ui components
- Responsive design with mobile support
- Accessibility features built-in
- Theme-aware styling

## Data Flow

1. **Client Request** → React components trigger API calls via TanStack Query
2. **API Layer** → Express routes handle HTTP requests and validate input
3. **Data Layer** → Drizzle ORM manages database operations
4. **ESP32 Communication** → HTTP clients communicate with physical devices
5. **Real-time Updates** → Polling-based updates every 5 seconds for live data

## External Dependencies

### Production Dependencies
- **@radix-ui/***: UI component primitives
- **@tanstack/react-query**: Server state management
- **drizzle-orm**: Database ORM
- **@neondatabase/serverless**: Database connection
- **react-hook-form**: Form state management
- **zod**: Runtime type validation
- **tailwindcss**: Utility-first CSS framework

### Development Dependencies
- **vite**: Build tool and development server
- **typescript**: Type checking
- **@types/node**: Node.js type definitions
- **drizzle-kit**: Database migration tool

## Deployment Strategy

### Development Environment
- **Local Development**: `npm run dev` starts both frontend and backend
- **Hot Reload**: Vite provides instant updates during development
- **Database**: Can use local PostgreSQL or Neon serverless
- **Simulators**: Test tools for ESP32 device simulation

### Production Environment
- **Build Process**: `npm run build` creates optimized production bundles
- **Server**: `npm start` runs the production server
- **Database**: PostgreSQL with connection pooling
- **Static Assets**: Served directly by Express in production

### Replit Configuration
- **Modules**: nodejs-20, web, postgresql-16
- **Run Command**: `npm run dev`
- **Build Command**: `npm run build`
- **Port Configuration**: 5000 (internal) → 80 (external)

## Changelog
- June 20, 2025. Initial setup
- June 20, 2025. Renamed dashboard page to master, updated routing from /dashboard to /master
- June 20, 2025. Created API_COMMUNICATION_PROTOCOL.md with complete ESP32 communication specifications

## User Preferences

Preferred communication style: Simple, everyday language.