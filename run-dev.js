#!/usr/bin/env node

// Set environment variables for development
process.env.NODE_ENV = 'development';
process.env.PORT = process.env.PORT || '5000';
process.env.HOST = process.env.HOST || '0.0.0.0';

// Import and run tsx programmatically
import { spawn } from 'child_process';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const serverPath = join(__dirname, 'server', 'index.ts');

console.log('Starting EV Charging Station Management System...');
console.log(`Environment: ${process.env.NODE_ENV}`);
console.log(`Server will run on: http://${process.env.HOST}:${process.env.PORT}`);
console.log('');

// Spawn tsx process
const child = spawn('npx', ['tsx', serverPath], {
  stdio: 'inherit',
  env: process.env,
  shell: true
});

child.on('error', (error) => {
  console.error('Failed to start server:', error);
  process.exit(1);
});

child.on('close', (code) => {
  process.exit(code);
});

// Handle graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down server...');
  child.kill('SIGINT');
});

process.on('SIGTERM', () => {
  child.kill('SIGTERM');
});