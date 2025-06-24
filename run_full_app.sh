#!/bin/bash

echo "Starting EV Charging Station Management System..."

# Kill any existing processes on ports 3000 and 5000
pkill -f "charging_station_server" 2>/dev/null || true
pkill -f "vite" 2>/dev/null || true

# Start C server in background
echo "Starting C backend server on port 5000..."
cd server_c
make debug >/dev/null 2>&1
PORT=5000 ./charging_station_server &
BACKEND_PID=$!

# Wait a moment for backend to start
sleep 2

# Go back to root and start frontend
cd ..
echo "Starting frontend development server on port 3000..."
npx vite --host 0.0.0.0 --port 3000 &
FRONTEND_PID=$!

echo "Backend PID: $BACKEND_PID"
echo "Frontend PID: $FRONTEND_PID"
echo ""
echo "🚀 Application running:"
echo "   Backend:  http://0.0.0.0:5000"
echo "   Frontend: http://0.0.0.0:3000"
echo ""
echo "Press Ctrl+C to stop both servers"

# Wait for either process to exit
wait $BACKEND_PID $FRONTEND_PID