#!/usr/bin/env node

/**
 * Startup proxy for C backend with integrated frontend
 * This file serves as a bridge to start both the C server and frontend dev server
 */

import { spawn } from 'child_process';
import { dirname, join } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const projectRoot = join(__dirname, '..');

console.log('🔄 Starting EV Charging Station Management System...');

// Build the C server
console.log('🏗️ Building C server...');
const buildProcess = spawn('make', ['debug'], {
  cwd: join(projectRoot, 'server_c'),
  stdio: 'inherit'
});

buildProcess.on('close', (code) => {
  if (code !== 0) {
    console.error('❌ Failed to build C server');
    process.exit(1);
  }
  
  console.log('✅ C server built successfully');
  
  // Start the C server on the correct port
  const serverPort = process.env.PORT || '5000';
  const serverProcess = spawn('./charging_station_server', [], {
    cwd: join(projectRoot, 'server_c'),
    stdio: 'inherit',
    env: { ...process.env, PORT: serverPort }
  });
  
  console.log('🚀 Backend server starting on port 5000...');
  
  // Build frontend for production serving
  setTimeout(() => {
    console.log('🏗️ Building frontend for production...');
    const buildProcess = spawn('npx', ['vite', 'build'], {
      cwd: projectRoot,
      stdio: 'inherit',
      env: { ...process.env }
    });
    
    buildProcess.on('close', (code) => {
      if (code === 0) {
        console.log('✅ Frontend built successfully - C server will serve static files');
      } else {
        console.log(`❌ Frontend build failed with code ${code}`);
      }
    });
  }, 1000);
  
  // Handle server exit
  serverProcess.on('close', (code) => {
    console.log(`Server process exited with code ${code}`);
    process.exit(code || 0);
  });
  
  // Handle termination signals
  process.on('SIGINT', () => {
    console.log('\n🛑 Shutting down servers...');
    serverProcess.kill('SIGINT');
  });
  
  process.on('SIGTERM', () => {
    console.log('\n🛑 Shutting down servers...');
    serverProcess.kill('SIGTERM');
  });
});