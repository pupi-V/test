#!/bin/bash

# Скрипт для запуска нескольких симуляторов плат одновременно

echo "=================================================="
echo "   ЗАПУСК СИМУЛЯТОРОВ ЗАРЯДНЫХ СТАНЦИЙ"
echo "=================================================="

# Проверяем, что сервер запущен
if ! curl -s http://localhost:5000/api/stations > /dev/null; then
    echo "❌ Сервер не запущен на порту 5000"
    echo "Запустите сервер командой: npm run dev"
    exit 1
fi

echo "✅ Сервер доступен"
echo

# Создаем директорию для логов
mkdir -p test/logs

# Запускаем симуляторы в фоновом режиме
echo "🚀 Запуск симуляторов..."

# Master плата (ID 1)
echo "Запуск Master платы (ID 1)..."
node test/board-simulator.cjs 1 master > test/logs/master-1.log 2>&1 &
MASTER_PID=$!

# Slave платы (ID 2, 3)
echo "Запуск Slave платы (ID 2)..."
node test/board-simulator.cjs 2 slave > test/logs/slave-2.log 2>&1 &
SLAVE2_PID=$!

echo "Запуск Slave платы (ID 3)..."
node test/board-simulator.cjs 3 slave > test/logs/slave-3.log 2>&1 &
SLAVE3_PID=$!

# Сохраняем PID процессов
echo $MASTER_PID > test/logs/master.pid
echo $SLAVE2_PID > test/logs/slave2.pid
echo $SLAVE3_PID > test/logs/slave3.pid

echo
echo "✅ Симуляторы запущены:"
echo "   Master (ID 1) - PID: $MASTER_PID"
echo "   Slave  (ID 2) - PID: $SLAVE2_PID"
echo "   Slave  (ID 3) - PID: $SLAVE3_PID"
echo
echo "📁 Логи сохраняются в test/logs/"
echo "🔍 Просмотр логов в реальном времени:"
echo "   tail -f test/logs/master-1.log"
echo "   tail -f test/logs/slave-2.log"
echo "   tail -f test/logs/slave-3.log"
echo
echo "🛑 Остановка всех симуляторов:"
echo "   ./test/stop-simulators.sh"
echo
echo "⏱️  Симуляторы будут работать до ручной остановки..."