
const fs = require('fs');
const path = require('path');

// Скрипт для конвертации React сборки в C заголовочные файлы для ESP32
function convertToHeader(inputPath, outputPath, variableName) {
  const content = fs.readFileSync(inputPath, 'utf8');
  const escaped = content
    .replace(/\\/g, '\\\\')
    .replace(/"/g, '\\"')
    .replace(/\n/g, '\\n')
    .replace(/\r/g, '\\r');
  
  const header = `#ifndef ${variableName.toUpperCase()}_H
#define ${variableName.toUpperCase()}_H

static const char ${variableName}[] = R"rawliteral(
${content}
)rawliteral";

#endif // ${variableName.toUpperCase()}_H
`;

  fs.writeFileSync(outputPath, header);
  console.log(`Создан файл: ${outputPath}`);
}

// Сборка React приложения
console.log('Собираем React приложение...');

// Создаем минимальную сборку для ESP32
const buildDir = './esp32_react_build';
if (!fs.existsSync(buildDir)) {
  fs.mkdirSync(buildDir);
}

// Копируем необходимые файлы из client/src
const reactHTML = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Charging Station</title>
  <style>
    /* Встроенные стили из index.css (минифицированные) */
    body{margin:0;font-family:system-ui,sans-serif;background:#f5f5f5}
    .container{max-width:800px;margin:0 auto;padding:20px}
    .card{background:white;border-radius:8px;padding:20px;margin:10px 0;box-shadow:0 2px 4px rgba(0,0,0,0.1)}
    .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px}
    .status{padding:4px 8px;border-radius:4px;font-size:12px;font-weight:bold}
    .status.charging{background:#4CAF50;color:white}
    .status.available{background:#2196F3;color:white}
    .status.offline{background:#f44336;color:white}
    .status.maintenance{background:#ff9800;color:white}
    button{padding:8px 16px;border:none;border-radius:4px;cursor:pointer;margin:4px}
    .primary{background:#2196F3;color:white}
    .danger{background:#f44336;color:white}
    input,select{padding:8px;border:1px solid #ddd;border-radius:4px;margin:4px}
    .checkbox{margin:8px 0}
    .power-table{width:100%;border-collapse:collapse}
    .power-table th,.power-table td{border:1px solid #ddd;padding:8px;text-align:center}
    .power-table input{width:80px;margin:0}
  </style>
</head>
<body>
  <div class="container">
    <div class="card">
      <h1>ESP32 Charging Station Control</h1>
      <div id="stationInfo">
        <div>Station ID: <span id="stationId">Loading...</span></div>
        <div>Type: <span id="stationType">slave</span></div>
        <div>Status: <span id="stationStatus" class="status">connecting</span></div>
        <div>Max Power: <span id="maxPower">0</span> kW</div>
      </div>
    </div>

    <div class="grid">
      <!-- Car Section -->
      <div class="card">
        <h3>Car Controls</h3>
        <div class="checkbox">
          <label>
            <input type="checkbox" id="carConnection"> Car Connected
          </label>
        </div>
        <div class="checkbox">
          <label>
            <input type="checkbox" id="carChargingPermission"> Charging Permission
          </label>
        </div>
        <div class="checkbox">
          <label>
            <input type="checkbox" id="carError"> Car Error
          </label>
        </div>
      </div>

      <!-- Master Section -->
      <div class="card">
        <h3>Master Status</h3>
        <div class="checkbox">
          <label>
            <input type="checkbox" id="masterOnline" disabled> Master Online
          </label>
        </div>
        <div class="checkbox">
          <label>
            <input type="checkbox" id="masterChargingPermission" disabled> Master Permission
          </label>
        </div>
        <div>
          <label>Available Power (kW):</label>
          <input type="number" id="masterAvailablePower" step="0.1" readonly>
        </div>
      </div>
    </div>

    <!-- Charger Section -->
    <div class="card">
      <h3>Charger Data</h3>
      
      <h4>Power Table</h4>
      <table class="power-table">
        <thead>
          <tr>
            <th>Phase</th>
            <th>1</th>
            <th>2</th>
            <th>3</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>Voltage (V)</td>
            <td><input type="number" id="voltagePhase1" step="0.1"></td>
            <td><input type="number" id="voltagePhase2" step="0.1"></td>
            <td><input type="number" id="voltagePhase3" step="0.1"></td>
          </tr>
          <tr>
            <td>Current (A)</td>
            <td><input type="number" id="currentPhase1" step="0.1"></td>
            <td><input type="number" id="currentPhase2" step="0.1"></td>
            <td><input type="number" id="currentPhase3" step="0.1"></td>
          </tr>
        </tbody>
      </table>

      <div style="margin-top: 20px;">
        <label>Total Power (kW):</label>
        <input type="number" id="chargerPower" step="0.1">
      </div>

      <h4>Status Flags</h4>
      <div class="checkbox">
        <label>
          <input type="checkbox" id="singlePhaseConnection"> Single Phase Connection
        </label>
      </div>
      <div class="checkbox">
        <label>
          <input type="checkbox" id="powerOverconsumption"> Power Overconsumption
        </label>
      </div>
      <div class="checkbox">
        <label>
          <input type="checkbox" id="fixedPower"> Fixed Power Mode
        </label>
      </div>
    </div>

    <!-- Controls -->
    <div class="card">
      <h3>Controls</h3>
      <button class="primary" onclick="saveData()">Save Settings</button>
      <button class="primary" onclick="toggleCharging()">Toggle Charging</button>
      <button class="danger" onclick="emergencyStop()">Emergency Stop</button>
      <button onclick="refreshData()">Refresh Data</button>
    </div>
  </div>

  <script>
    // React-like state management для ESP32
    let stationData = {
      carConnection: false,
      carChargingPermission: false,
      carError: false,
      masterOnline: false,
      masterChargingPermission: false,
      masterAvailablePower: 0,
      voltagePhase1: 220,
      voltagePhase2: 220,
      voltagePhase3: 220,
      currentPhase1: 0,
      currentPhase2: 0,
      currentPhase3: 0,
      chargerPower: 0,
      singlePhaseConnection: false,
      powerOverconsumption: false,
      fixedPower: false
    };

    // Автосохранение (как в React приложении)
    let autoSaveTimeout = null;

    function scheduleAutoSave() {
      if (autoSaveTimeout) {
        clearTimeout(autoSaveTimeout);
      }
      autoSaveTimeout = setTimeout(() => {
        saveData();
      }, 2000);
    }

    // Обновление данных из формы
    function updateStateFromForm() {
      stationData.carConnection = document.getElementById('carConnection').checked;
      stationData.carChargingPermission = document.getElementById('carChargingPermission').checked;
      stationData.carError = document.getElementById('carError').checked;
      stationData.voltagePhase1 = parseFloat(document.getElementById('voltagePhase1').value) || 0;
      stationData.voltagePhase2 = parseFloat(document.getElementById('voltagePhase2').value) || 0;
      stationData.voltagePhase3 = parseFloat(document.getElementById('voltagePhase3').value) || 0;
      stationData.currentPhase1 = parseFloat(document.getElementById('currentPhase1').value) || 0;
      stationData.currentPhase2 = parseFloat(document.getElementById('currentPhase2').value) || 0;
      stationData.currentPhase3 = parseFloat(document.getElementById('currentPhase3').value) || 0;
      stationData.chargerPower = parseFloat(document.getElementById('chargerPower').value) || 0;
      stationData.singlePhaseConnection = document.getElementById('singlePhaseConnection').checked;
      stationData.powerOverconsumption = document.getElementById('powerOverconsumption').checked;
      stationData.fixedPower = document.getElementById('fixedPower').checked;
    }

    // Обновление формы из данных
    function updateFormFromState() {
      document.getElementById('carConnection').checked = stationData.carConnection;
      document.getElementById('carChargingPermission').checked = stationData.carChargingPermission;
      document.getElementById('carError').checked = stationData.carError;
      document.getElementById('masterOnline').checked = stationData.masterOnline;
      document.getElementById('masterChargingPermission').checked = stationData.masterChargingPermission;
      document.getElementById('masterAvailablePower').value = stationData.masterAvailablePower;
      document.getElementById('voltagePhase1').value = stationData.voltagePhase1;
      document.getElementById('voltagePhase2').value = stationData.voltagePhase2;
      document.getElementById('voltagePhase3').value = stationData.voltagePhase3;
      document.getElementById('currentPhase1').value = stationData.currentPhase1;
      document.getElementById('currentPhase2').value = stationData.currentPhase2;
      document.getElementById('currentPhase3').value = stationData.currentPhase3;
      document.getElementById('chargerPower').value = stationData.chargerPower;
      document.getElementById('singlePhaseConnection').checked = stationData.singlePhaseConnection;
      document.getElementById('powerOverconsumption').checked = stationData.powerOverconsumption;
      document.getElementById('fixedPower').checked = stationData.fixedPower;
    }

    // API функции
    async function fetchStationData() {
      try {
        const response = await fetch('/api/data');
        if (response.ok) {
          const data = await response.json();
          Object.assign(stationData, data.slave_data || {});
          updateFormFromState();
          
          // Обновляем статус
          document.getElementById('stationStatus').textContent = data.status || 'unknown';
          document.getElementById('stationStatus').className = 'status ' + (data.status || 'offline');
        }
      } catch (error) {
        console.error('Error fetching data:', error);
        document.getElementById('stationStatus').textContent = 'error';
        document.getElementById('stationStatus').className = 'status offline';
      }
    }

    async function saveData() {
      updateStateFromForm();
      try {
        const response = await fetch('/api/data', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            command: 'update_slave_data',
            slave_data: stationData
          })
        });
        
        if (response.ok) {
          console.log('Data saved successfully');
          document.getElementById('stationStatus').textContent = 'saved';
          setTimeout(() => {
            fetchStationData();
          }, 500);
        }
      } catch (error) {
        console.error('Error saving data:', error);
      }
    }

    async function toggleCharging() {
      try {
        await fetch('/api/data', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ command: 'toggle_charging' })
        });
        setTimeout(fetchStationData, 500);
      } catch (error) {
        console.error('Error:', error);
      }
    }

    async function emergencyStop() {
      if (confirm('Emergency stop?')) {
        try {
          await fetch('/api/data', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command: 'emergency_stop' })
          });
          setTimeout(fetchStationData, 500);
        } catch (error) {
          console.error('Error:', error);
        }
      }
    }

    function refreshData() {
      fetchStationData();
    }

    // Инициализация (как useEffect в React)
    document.addEventListener('DOMContentLoaded', async () => {
      // Загружаем информацию о станции
      try {
        const infoResponse = await fetch('/api/info');
        if (infoResponse.ok) {
          const info = await infoResponse.json();
          document.getElementById('stationId').textContent = info.board_id || 'ESP32-UNKNOWN';
          document.getElementById('stationType').textContent = info.board_type || 'slave';
          document.getElementById('maxPower').textContent = info.max_power || 22;
        }
      } catch (error) {
        console.error('Error loading station info:', error);
      }

      // Добавляем обработчики автосохранения (как в React)
      const inputs = document.querySelectorAll('input[type="checkbox"], input[type="number"]');
      inputs.forEach(input => {
        input.addEventListener('change', () => {
          if (input.type === 'checkbox') {
            // Мгновенное сохранение для чекбоксов
            saveData();
          } else {
            // Автосохранение для числовых полей
            scheduleAutoSave();
          }
        });
      });

      // Первоначальная загрузка данных
      fetchStationData();
    });

    // Автообновление каждые 5 секунд (как в React)
    setInterval(fetchStationData, 5000);
  </script>
</body>
</html>`;

// Сохраняем HTML файл
fs.writeFileSync(path.join(buildDir, 'esp32_react_interface.html'), reactHTML);

// Конвертируем в C заголовочный файл
convertToHeader(
  path.join(buildDir, 'esp32_react_interface.html'),
  path.join(buildDir, 'web_interface_react.h'),
  'web_interface_react_html'
);

console.log('\nГотовые файлы для ESP32:');
console.log('- esp32_react_build/web_interface_react.h');
console.log('\nСкопируйте этот файл в папку main/ вашего ESP32 проекта');
