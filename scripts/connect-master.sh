#!/bin/bash
echo "🔌 Подключение Master платы..."
node scripts/emulate-board.js master
echo "✅ Обновите страницу для автоматического перехода к dashboard"