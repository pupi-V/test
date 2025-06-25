#!/bin/bash

echo "Starting EV Charging Station Management System..."

# Kill existing processes
pkill -f "charging_station_server" 2>/dev/null || true
pkill -f "vite" 2>/dev/null || true

# Build and start C server
cd server_c
make debug >/dev/null 2>&1
echo "Starting C backend server on port 5000..."
PORT=5000 ./charging_station_server &
BACKEND_PID=$!

# Start frontend dev server
cd ..
echo "Starting frontend development server..."
npx vite --host 0.0.0.0 --port 3000 &
FRONTEND_PID=$!

echo "Application ready:"
echo "  Frontend: http://localhost:3000"
echo "  Backend API: http://localhost:5000"
echo ""
echo "Press Ctrl+C to stop both servers"

# Handle shutdown
trap 'echo "Shutting down..."; kill $BACKEND_PID $FRONTEND_PID 2>/dev/null; exit' INT TERM

# Keep script running
wait $BACKEND_PID $FRONTEND_PID