#!/bin/bash

echo "🚀 Демонстрация сценариев подключения плат"
echo "========================================"

echo ""
echo "📋 Доступные команды для эмуляции:"
echo "  ./scripts/connect-master.sh    - Подключить Master плату"
echo "  ./scripts/connect-slave.sh     - Подключить Slave плату" 
echo "  ./scripts/disconnect-all.sh    - Отключить все платы"
echo ""
echo "  node scripts/emulate-board.js master   - Master плата"
echo "  node scripts/emulate-board.js slave    - Slave плата"
echo "  node scripts/emulate-board.js none     - Нет плат"
echo "  node scripts/emulate-board.js both     - Обе платы"
echo ""

echo "🔍 Текущее состояние системы:"
if [ -f "data/stations.json" ]; then
    STATIONS_COUNT=$(node -e "
        try {
            const fs = require('fs');
            const data = JSON.parse(fs.readFileSync('data/stations.json', 'utf-8'));
            console.log(\`Станций: \${data.length}\`);
            data.forEach(s => console.log(\`  ID \${s.id}: \${s.type.toUpperCase()} - \${s.displayName || 'Без названия'}\`));
        } catch(e) {
            console.log('Ошибка чтения данных');
        }
    ")
    echo "$STATIONS_COUNT"
else
    echo "  Файл данных отсутствует"
fi

echo ""
echo "💡 Для изменения состояния используйте команды выше"
echo "   После каждой команды обновите страницу браузера"