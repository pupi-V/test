#!/bin/bash
echo "🔌 Подключение Slave платы..."
node scripts/emulate-board.js slave
echo "✅ Обновите страницу для автоматического перехода к интерфейсу станции"