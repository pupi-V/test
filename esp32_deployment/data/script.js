// Глобальные переменные
let stations = [];
let currentEditingStationId = null;

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', function() {
    initializeApp();
});

function initializeApp() {
    setupEventListeners();
    loadStations();
    displayESP32Info();
}

function setupEventListeners() {
    // Кнопки управления
    document.getElementById('refresh-btn').addEventListener('click', loadStations);
    document.getElementById('scan-btn').addEventListener('click', scanESP32Boards);
    document.getElementById('add-station-btn').addEventListener('click', openAddModal);
    
    // Формы
    document.getElementById('add-station-form').addEventListener('submit', handleAddStation);
    document.getElementById('edit-station-form').addEventListener('submit', handleEditStation);
    
    // Закрытие модальных окон
    document.querySelectorAll('.close').forEach(close => {
        close.addEventListener('click', closeModals);
    });
    
    // Закрытие модальных окон при клике вне них
    window.addEventListener('click', function(event) {
        if (event.target.classList.contains('modal')) {
            closeModals();
        }
    });
}

function displayESP32Info() {
    // Отображение IP адреса ESP32
    document.getElementById('esp32-ip').textContent = `ESP32: ${window.location.hostname}`;
}

async function loadStations() {
    try {
        showLoading();
        const response = await fetch('/api/stations');
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        stations = await response.json();
        renderStations();
        showSuccess('Станции загружены успешно');
    } catch (error) {
        console.error('Ошибка загрузки станций:', error);
        showError('Ошибка загрузки станций: ' + error.message);
    }
}

function renderStations() {
    const container = document.getElementById('stations-container');
    
    if (stations.length === 0) {
        container.innerHTML = `
            <div class="loading">
                <h3>Станции не найдены</h3>
                <p>Добавьте первую станцию для начала работы</p>
            </div>
        `;
        return;
    }
    
    container.innerHTML = stations.map(station => `
        <div class="station-card">
            <div class="station-header">
                <div class="station-name">${station.displayName}</div>
                <div class="station-type type-${station.type}">${station.type.toUpperCase()}</div>
            </div>
            
            <div class="station-status status-${station.status}">${getStatusLabel(station.status)}</div>
            
            <div class="station-details">
                <div class="detail-row">
                    <span>Техническое имя:</span>
                    <span>${station.technicalName}</span>
                </div>
                <div class="detail-row">
                    <span>Максимальная мощность:</span>
                    <span>${station.maxPower} кВт</span>
                </div>
                <div class="detail-row">
                    <span>Текущая мощность:</span>
                    <span>${station.currentPower} кВт</span>
                </div>
                <div class="detail-row">
                    <span>Автомобиль:</span>
                    <span>${station.carConnection ? '🟢 Подключен' : '🔴 Отключен'}</span>
                </div>
                <div class="detail-row">
                    <span>Разрешение зарядки:</span>
                    <span>${station.carChargingPermission ? '🟢 Разрешено' : '🔴 Запрещено'}</span>
                </div>
                ${station.type === 'slave' ? `
                <div class="detail-row">
                    <span>Мастер онлайн:</span>
                    <span>${station.masterOnline ? '🟢 Да' : '🔴 Нет'}</span>
                </div>
                <div class="detail-row">
                    <span>Доступная мощность:</span>
                    <span>${station.masterAvailablePower} кВт</span>
                </div>
                ` : ''}
            </div>
            
            <div class="station-actions">
                <button class="btn btn-primary" onclick="openEditModal(${station.id})">✏️ Редактировать</button>
                <button class="btn btn-warning" onclick="toggleCharging(${station.id})">
                    ${station.carChargingPermission ? '⏸️ Остановить' : '▶️ Запустить'}
                </button>
            </div>
        </div>
    `).join('');
}

function getStatusLabel(status) {
    const labels = {
        'available': 'Доступна',
        'charging': 'Заряжается',
        'offline': 'Отключена',
        'online': 'Онлайн',
        'maintenance': 'Обслуживание'
    };
    return labels[status] || status;
}

async function scanESP32Boards() {
    try {
        showMessage('Сканирование ESP32 плат...', 'info');
        const response = await fetch('/api/esp32/scan', { method: 'POST' });
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const boards = await response.json();
        
        if (boards.length === 0) {
            showMessage('ESP32 платы не найдены', 'warning');
        } else {
            showSuccess(`Найдено ${boards.length} ESP32 плат`);
            console.log('Найденные платы:', boards);
        }
    } catch (error) {
        console.error('Ошибка сканирования:', error);
        showError('Ошибка сканирования: ' + error.message);
    }
}

function openAddModal() {
    document.getElementById('add-station-modal').style.display = 'block';
}

function openEditModal(stationId) {
    const station = stations.find(s => s.id === stationId);
    if (!station) return;
    
    currentEditingStationId = stationId;
    
    document.getElementById('edit-station-id').value = station.id;
    document.getElementById('edit-display-name').value = station.displayName;
    document.getElementById('edit-max-power').value = station.maxPower;
    document.getElementById('edit-current-power').value = station.currentPower;
    document.getElementById('edit-status').value = station.status;
    document.getElementById('edit-car-connection').checked = station.carConnection;
    
    document.getElementById('edit-station-modal').style.display = 'block';
}

function closeModals() {
    document.getElementById('add-station-modal').style.display = 'none';
    document.getElementById('edit-station-modal').style.display = 'none';
    currentEditingStationId = null;
}

function closeModal() {
    closeModals();
}

function closeEditModal() {
    closeModals();
}

async function handleAddStation(event) {
    event.preventDefault();
    
    const formData = {
        displayName: document.getElementById('display-name').value,
        technicalName: document.getElementById('technical-name').value,
        type: document.getElementById('type').value,
        maxPower: parseFloat(document.getElementById('max-power').value)
    };
    
    try {
        const response = await fetch('/api/stations', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(formData)
        });
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        showSuccess('Станция создана успешно');
        closeModals();
        document.getElementById('add-station-form').reset();
        loadStations();
    } catch (error) {
        console.error('Ошибка создания станции:', error);
        showError('Ошибка создания станции: ' + error.message);
    }
}

async function handleEditStation(event) {
    event.preventDefault();
    
    if (!currentEditingStationId) return;
    
    const formData = {
        id: currentEditingStationId,
        displayName: document.getElementById('edit-display-name').value,
        maxPower: parseFloat(document.getElementById('edit-max-power').value),
        currentPower: parseFloat(document.getElementById('edit-current-power').value),
        status: document.getElementById('edit-status').value,
        carConnection: document.getElementById('edit-car-connection').checked
    };
    
    try {
        const response = await fetch('/api/stations/update', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(formData)
        });
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        showSuccess('Станция обновлена успешно');
        closeModals();
        loadStations();
    } catch (error) {
        console.error('Ошибка обновления станции:', error);
        showError('Ошибка обновления станции: ' + error.message);
    }
}

async function toggleCharging(stationId) {
    const station = stations.find(s => s.id === stationId);
    if (!station) return;
    
    const newPermission = !station.carChargingPermission;
    
    try {
        const response = await fetch('/api/stations/update', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                id: stationId,
                carChargingPermission: newPermission,
                status: newPermission ? 'charging' : 'available'
            })
        });
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        showSuccess(newPermission ? 'Зарядка запущена' : 'Зарядка остановлена');
        loadStations();
    } catch (error) {
        console.error('Ошибка управления зарядкой:', error);
        showError('Ошибка управления зарядкой: ' + error.message);
    }
}

function showLoading() {
    document.getElementById('stations-container').innerHTML = `
        <div class="loading">
            <h3>⏳ Загрузка...</h3>
            <p>Получение данных о станциях</p>
        </div>
    `;
}

function showMessage(message, type = 'info') {
    // Удаляем предыдущие сообщения
    const existingMessages = document.querySelectorAll('.message');
    existingMessages.forEach(msg => msg.remove());
    
    const messageDiv = document.createElement('div');
    messageDiv.className = `message ${type}`;
    messageDiv.textContent = message;
    
    document.querySelector('.container').insertBefore(messageDiv, document.querySelector('main'));
    
    // Автоматическое скрытие через 5 секунд
    setTimeout(() => {
        messageDiv.remove();
    }, 5000);
}

function showError(message) {
    showMessage(message, 'error');
}

function showSuccess(message) {
    showMessage(message, 'success');
}