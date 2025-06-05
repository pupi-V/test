#!/bin/bash

# Скрипт для остановки всех запущенных симуляторов

echo "🛑 Остановка симуляторов плат..."

# Проверяем наличие файлов с PID
if [ -f "test/logs/master.pid" ]; then
    MASTER_PID=$(cat test/logs/master.pid)
    if kill -0 $MASTER_PID 2>/dev/null; then
        kill $MASTER_PID
        echo "✅ Master симулятор (PID: $MASTER_PID) остановлен"
    else
        echo "⚠️  Master симулятор уже не работает"
    fi
    rm -f test/logs/master.pid
fi

if [ -f "test/logs/slave2.pid" ]; then
    SLAVE2_PID=$(cat test/logs/slave2.pid)
    if kill -0 $SLAVE2_PID 2>/dev/null; then
        kill $SLAVE2_PID
        echo "✅ Slave симулятор 2 (PID: $SLAVE2_PID) остановлен"
    else
        echo "⚠️  Slave симулятор 2 уже не работает"
    fi
    rm -f test/logs/slave2.pid
fi

if [ -f "test/logs/slave3.pid" ]; then
    SLAVE3_PID=$(cat test/logs/slave3.pid)
    if kill -0 $SLAVE3_PID 2>/dev/null; then
        kill $SLAVE3_PID
        echo "✅ Slave симулятор 3 (PID: $SLAVE3_PID) остановлен"
    else
        echo "⚠️  Slave симулятор 3 уже не работает"
    fi
    rm -f test/logs/slave3.pid
fi

# Дополнительная очистка процессов node с симуляторами
pkill -f "board-simulator.js" 2>/dev/null

echo "🧹 Очистка завершена"