#!/bin/bash
# Start the EV Charging Station Management System
# Backend: C server on port 5000
# Frontend: Vite dev server on port 3000

echo "Starting EV Charging Station Management System..."

# Start backend C server
cd server_c
make debug > /dev/null 2>&1
echo "Backend starting on port 5000..."
PORT=5000 ./charging_station_server &
BACKEND_PID=$!

# Start frontend 
cd ..
echo "Frontend starting on port 3000..."
npx vite --host 0.0.0.0 --port 3000 &
FRONTEND_PID=$!

echo "Application ready at http://localhost:3000"
echo "API available at http://localhost:5000"

# Keep script running
wait $BACKEND_PID $FRONTEND_PID