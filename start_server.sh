#!/bin/bash

# Start the C server for the EV charging station management system
cd server_c
make debug
PORT=5000 ./charging_station_server