import { spawn } from 'child_process';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));

process.env.NODE_ENV = process.env.NODE_ENV || 'development';
process.env.PORT = process.env.PORT || '5000';
process.env.HOST = process.env.HOST || '0.0.0.0';

console.log('🚀 Starting EV Charging Station Management System...');
console.log(`📍 Environment: ${process.env.NODE_ENV}`);
console.log(`🌐 Server: http://${process.env.HOST}:${process.env.PORT}`);
console.log(`💻 Local access: http://localhost:${process.env.PORT}`);
console.log('');

const tsx = spawn('npx', ['tsx', join(__dirname, 'server', 'index.ts')], {
  stdio: 'inherit',
  env: process.env,
  shell: true
});

tsx.on('error', (error) => {
  console.error('❌ Failed to start:', error.message);
  process.exit(1);
});

process.on('SIGINT', () => {
  console.log('\n🛑 Shutting down...');
  tsx.kill();
  process.exit(0);
});