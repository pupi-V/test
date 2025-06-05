// JavaScript для управления зарядными станциями ESP32
class ChargingStationManager {
    constructor() {
        this.currentTheme = 'light';
        this.selectedStation = 1;
        this.stations = new Map();
        this.lastUpdate = null;
        this.refreshInterval = null;
        
        this.init();
    }

    init() {
        this.setupEventListeners();
        this.loadInitialData();
        this.startAutoRefresh();
        this.updateLastUpdateTime();
    }

    setupEventListeners() {
        // Обновление времени каждую секунду
        setInterval(() => this.updateLastUpdateTime(), 1000);
    }

    // Управление вкладками
    showTab(tabId) {
        // Скрываем все вкладки
        document.querySelectorAll('.tab-content').forEach(tab => {
            tab.classList.remove('active');
        });
        
        // Убираем активность с кнопок
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.classList.remove('active');
        });
        
        // Показываем выбранную вкладку
        document.getElementById(tabId).classList.add('active');
        
        // Активируем соответствующую кнопку
        event.target.classList.add('active');
    }

    // Переключение темы
    toggleTheme() {
        this.currentTheme = this.currentTheme === 'light' ? 'dark' : 'light';
        document.body.classList.toggle('dark');
        
        const themeBtn = document.getElementById('theme-btn');
        themeBtn.textContent = this.currentTheme === 'light' ? '🌙' : '☀️';
        
        // Сохраняем тему в localStorage
        localStorage.setItem('theme', this.currentTheme);
    }

    // Загрузка начальных данных
    async loadInitialData() {
        try {
            // Загружаем сохраненную тему
            const savedTheme = localStorage.getItem('theme');
            if (savedTheme && savedTheme === 'dark') {
                this.toggleTheme();
            }

            // Загружаем данные станций
            await this.refreshStationData();
        } catch (error) {
            console.error('Ошибка загрузки начальных данных:', error);
            this.updateConnectionStatus('Ошибка подключения', false);
        }
    }

    // Сканирование ESP32 плат
    async scanBoards() {
        const scanBtn = document.getElementById('scan-btn');
        const scanResults = document.getElementById('scan-results');
        
        scanBtn.disabled = true;
        scanBtn.textContent = '🔍 Сканирование...';
        scanResults.innerHTML = '<p>Поиск ESP32 плат в сети...</p>';

        try {
            // Запрос к серверу для сканирования
            const response = await fetch('/api/esp32/scan', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                }
            });

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const boards = await response.json();
            this.displayScanResults(boards);
            
        } catch (error) {
            console.error('Ошибка сканирования:', error);
            scanResults.innerHTML = `<p style="color: var(--danger-color);">Ошибка сканирования: ${error.message}</p>`;
        } finally {
            scanBtn.disabled = false;
            scanBtn.textContent = 'Сканировать сеть';
        }
    }

    // Отображение результатов сканирования
    displayScanResults(boards) {
        const scanResults = document.getElementById('scan-results');
        
        if (boards.length === 0) {
            scanResults.innerHTML = '<p>ESP32 платы не найдены. Проверьте подключение к сети.</p>';
            return;
        }

        const boardsHtml = boards.map(board => `
            <div class="board-item" style="margin: 0.5rem 0; padding: 1rem; border: 1px solid var(--border-color); border-radius: 8px;">
                <div style="display: flex; justify-content: space-between; align-items: center;">
                    <div>
                        <strong>${board.name}</strong> (${board.type})
                        <br>
                        <small>IP: ${board.ip} | Статус: ${board.status}</small>
                    </div>
                    <button onclick="stationManager.connectToSpecificBoard('${board.ip}')" 
                            style="background: var(--primary-color); color: white; border: none; padding: 0.5rem 1rem; border-radius: 6px; cursor: pointer;">
                        Подключить
                    </button>
                </div>
            </div>
        `).join('');

        scanResults.innerHTML = `
            <h4>Найдено плат: ${boards.length}</h4>
            ${boardsHtml}
        `;
    }

    // Подключение к плате по IP
    async connectToBoard() {
        const ipInput = document.getElementById('manual-ip');
        const ip = ipInput.value.trim();
        
        if (!ip) {
            alert('Введите IP адрес платы');
            return;
        }

        await this.connectToSpecificBoard(ip);
    }

    // Подключение к конкретной плате
    async connectToSpecificBoard(ip) {
        try {
            const response = await fetch('/api/esp32/connect', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ ip })
            });

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const result = await response.json();
            
            if (result.success) {
                alert(`Успешно подключено к плате ${ip}`);
                await this.refreshStationData();
            } else {
                alert(`Ошибка подключения: ${result.error}`);
            }
            
        } catch (error) {
            console.error('Ошибка подключения к плате:', error);
            alert(`Ошибка подключения: ${error.message}`);
        }
    }

    // Выбор станции для управления
    selectStation(stationId) {
        this.selectedStation = stationId;
        this.showTab('station-control');
        
        // Обновляем номер станции в интерфейсе
        document.getElementById('selected-station').textContent = stationId;
        
        // Загружаем данные выбранной станции
        this.loadStationConfig(stationId);
    }

    // Загрузка конфигурации станции
    async loadStationConfig(stationId) {
        try {
            const response = await fetch(`/api/stations/${stationId}`);
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const station = await response.json();
            
            // Заполняем форму данными станции
            document.getElementById('station-name').value = station.displayName || `Станция ${stationId}`;
            document.getElementById('max-power').value = station.maxPower || 22;
            document.getElementById('station-type').value = station.typ || 'master';
            document.getElementById('charging-status').value = station.status || 'available';
            document.getElementById('current-power').value = station.currentPower || 0;
            
        } catch (error) {
            console.error('Ошибка загрузки конфигурации станции:', error);
        }
    }

    // Сохранение конфигурации станции
    async saveStationConfig() {
        const stationData = {
            displayName: document.getElementById('station-name').value,
            maxPower: parseFloat(document.getElementById('max-power').value),
            typ: document.getElementById('station-type').value,
            status: document.getElementById('charging-status').value,
            currentPower: parseFloat(document.getElementById('current-power').value)
        };

        try {
            const response = await fetch(`/api/stations/${this.selectedStation}`, {
                method: 'PATCH',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(stationData)
            });

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            alert('Настройки сохранены успешно');
            await this.refreshStationData();
            
        } catch (error) {
            console.error('Ошибка сохранения настроек:', error);
            alert(`Ошибка сохранения: ${error.message}`);
        }
    }

    // Сброс станции
    async resetStation() {
        if (!confirm('Вы уверены, что хотите сбросить настройки станции?')) {
            return;
        }

        try {
            // Отправляем команду сброса на ESP32
            const response = await fetch(`/api/esp32/${this.selectedStation}/reset`, {
                method: 'POST'
            });

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            alert('Станция сброшена');
            await this.refreshStationData();
            
        } catch (error) {
            console.error('Ошибка сброса станции:', error);
            alert(`Ошибка сброса: ${error.message}`);
        }
    }

    // Обновление данных
    async refreshData() {
        await this.refreshStationData();
        alert('Данные обновлены');
    }

    // Обновление данных станций
    async refreshStationData() {
        try {
            const response = await fetch('/api/stations');
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const stations = await response.json();
            this.updateDashboard(stations);
            this.updateConnectionStatus('Подключено', true);
            
        } catch (error) {
            console.error('Ошибка обновления данных:', error);
            this.updateConnectionStatus('Ошибка подключения', false);
        }
    }

    // Обновление дашборда
    updateDashboard(stations) {
        stations.forEach((station, index) => {
            const powerElement = document.getElementById(`power${index + 1}`);
            if (powerElement) {
                powerElement.textContent = station.currentPower || 0;
            }
        });

        this.lastUpdate = new Date();
    }

    // Обновление статуса подключения
    updateConnectionStatus(status, isConnected) {
        const statusElement = document.getElementById('connection-status');
        statusElement.textContent = isConnected ? '🟢 ' + status : '🔴 ' + status;
    }

    // Обновление времени последнего обновления
    updateLastUpdateTime() {
        const timeElement = document.getElementById('last-update');
        if (this.lastUpdate) {
            timeElement.textContent = this.lastUpdate.toLocaleTimeString();
        } else {
            timeElement.textContent = '--:--';
        }
    }

    // Автоматическое обновление данных
    startAutoRefresh() {
        this.refreshInterval = setInterval(() => {
            this.refreshStationData();
        }, 5000); // Обновление каждые 5 секунд
    }

    // Остановка автоматического обновления
    stopAutoRefresh() {
        if (this.refreshInterval) {
            clearInterval(this.refreshInterval);
            this.refreshInterval = null;
        }
    }
}

// Глобальные функции для HTML
let stationManager;

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', function() {
    stationManager = new ChargingStationManager();
});

// Глобальные функции для доступа из HTML
function showTab(tabId) {
    stationManager.showTab(tabId);
}

function toggleTheme() {
    stationManager.toggleTheme();
}

function scanBoards() {
    stationManager.scanBoards();
}

function connectToBoard() {
    stationManager.connectToBoard();
}

function selectStation(stationId) {
    stationManager.selectStation(stationId);
}

function saveStationConfig() {
    stationManager.saveStationConfig();
}

function resetStation() {
    stationManager.resetStation();
}

function refreshData() {
    stationManager.refreshData();
}