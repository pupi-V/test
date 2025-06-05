// Импорты для работы с Express сервером и HTTP
import type { Express } from "express";
import { createServer, type Server } from "http";
import { storage } from "./storage";
import { insertChargingStationSchema, updateChargingStationSchema } from "@shared/schema";
import { scanForESP32Boards, connectToESP32Board, sendDataToESP32, getDataFromESP32 } from "./esp32-client";

/**
 * Регистрирует все API маршруты для работы с зарядными станциями
 * @param app - экземпляр Express приложения
 * @returns HTTP сервер
 */
export async function registerRoutes(app: Express): Promise<Server> {
  
  /**
   * GET /api/stations
   * Получает список всех зарядных станций
   */
  app.get("/api/stations", async (req, res) => {
    try {
      const stations = await storage.getChargingStations();
      res.json(stations);
    } catch (error) {
      console.error("Error fetching stations:", error);
      res.status(500).json({ message: "Failed to fetch stations" });
    }
  });

  /**
   * GET /api/stations/:id
   * Получает конкретную зарядную станцию по ID
   */
  app.get("/api/stations/:id", async (req, res) => {
    try {
      const id = parseInt(req.params.id);
      const station = await storage.getChargingStation(id);
      
      if (!station) {
        return res.status(404).json({ message: "Station not found" });
      }
      
      res.json(station);
    } catch (error) {
      console.error("Error fetching station:", error);
      res.status(500).json({ message: "Failed to fetch station" });
    }
  });

  /**
   * POST /api/stations
   * Создает новую зарядную станцию
   * Валидирует входные данные перед сохранением
   */
  app.post("/api/stations", async (req, res) => {
    try {
      // Валидируем данные с помощью Zod схемы
      const validatedData = insertChargingStationSchema.parse(req.body);
      const station = await storage.createChargingStation(validatedData);
      res.status(201).json(station);
    } catch (error: any) {
      // Проверяем тип ошибки валидации
      if (error.name === "ZodError") {
        return res.status(400).json({ 
          message: "Invalid station data", 
          errors: error.errors 
        });
      }
      console.error("Error creating station:", error);
      res.status(500).json({ message: "Failed to create station" });
    }
  });

  /**
   * PATCH /api/stations/:id
   * Обновляет существующую зарядную станцию
   * Поддерживает частичное обновление полей
   */
  app.patch("/api/stations/:id", async (req, res) => {
    try {
      const id = parseInt(req.params.id);
      // Валидируем данные для обновления
      const validatedData = updateChargingStationSchema.parse(req.body);
      const station = await storage.updateChargingStation(id, validatedData);
      
      if (!station) {
        return res.status(404).json({ message: "Station not found" });
      }
      
      res.json(station);
    } catch (error: any) {
      if (error.name === "ZodError") {
        return res.status(400).json({ 
          message: "Invalid station data", 
          errors: error.errors 
        });
      }
      console.error("Error updating station:", error);
      res.status(500).json({ message: "Failed to update station" });
    }
  });

  /**
   * DELETE /api/stations/:id
   * Удаляет зарядную станцию по ID
   */
  app.delete("/api/stations/:id", async (req, res) => {
    try {
      const id = parseInt(req.params.id);
      const deleted = await storage.deleteChargingStation(id);
      
      if (!deleted) {
        return res.status(404).json({ message: "Station not found" });
      }
      
      // 204 означает успешное удаление без содержимого в ответе
      res.status(204).send();
    } catch (error) {
      console.error("Error deleting station:", error);
      res.status(500).json({ message: "Failed to delete station" });
    }
  });



  /**
   * POST /api/board/connect
   * Подключение платы - определение типа платы и возврат соответствующих данных
   */
  app.post("/api/board/connect", async (req, res) => {
    try {
      const { boardId } = req.body;
      
      if (!boardId) {
        return res.status(400).json({ message: "Board ID is required" });
      }
      
      // Ищем плату по ID
      const station = await storage.getChargingStation(boardId);
      
      if (!station) {
        return res.status(404).json({ message: "Board not found" });
      }
      
      // Возвращаем информацию о плате и её данные
      const response = {
        id: station.id,
        type: station.type,
        displayName: station.displayName,
        technicalName: station.technicalName,
        status: station.status,
        maxPower: station.maxPower,
        currentPower: station.currentPower,
        // Для slave-плат возвращаем дополнительные данные
        ...(station.type === "slave" && {
          carConnection: station.carConnection,
          carChargingPermission: station.carChargingPermission,
          carError: station.carError,
          masterOnline: station.masterOnline,
          masterChargingPermission: station.masterChargingPermission,
          masterAvailablePower: station.masterAvailablePower,
          voltagePhase1: station.voltagePhase1,
          voltagePhase2: station.voltagePhase2,
          voltagePhase3: station.voltagePhase3,
          currentPhase1: station.currentPhase1,
          currentPhase2: station.currentPhase2,
          currentPhase3: station.currentPhase3,
          chargerPower: station.chargerPower,
          singlePhaseConnection: station.singlePhaseConnection,
          powerOverconsumption: station.powerOverconsumption,
          fixedPower: station.fixedPower
        })
      };
      
      res.json(response);
    } catch (error) {
      console.error("Error connecting board:", error);
      res.status(500).json({ message: "Failed to connect board" });
    }
  });

  /**
   * POST /api/esp32/scan
   * Сканирование локальной сети для поиска плат ESP32
   */
  app.post("/api/esp32/scan", async (req, res) => {
    try {
      console.log('Начинаем сканирование сети для поиска ESP32 плат...');
      const foundBoards = await scanForESP32Boards();
      
      console.log(`Сканирование завершено. Найдено плат: ${foundBoards.length}`);
      res.json(foundBoards);
    } catch (error) {
      console.error('Ошибка сканирования ESP32:', error);
      res.status(500).json({ 
        error: "Ошибка при сканировании сети" 
      });
    }
  });

  /**
   * POST /api/esp32/connect
   * Подключение к конкретной плате ESP32 по IP адресу
   */
  app.post("/api/esp32/connect", async (req, res) => {
    try {
      const { ip, type } = req.body;
      
      if (!ip) {
        return res.status(400).json({ 
          error: "Требуется указать IP адрес платы" 
        });
      }

      console.log(`Попытка подключения к ESP32 плате ${ip}${type ? ` (тип: ${type})` : ''}...`);

      // Проверяем доступность платы и получаем её конфигурацию
      const boardInfo = await connectToESP32Board(ip, type);
      
      if (!boardInfo) {
        return res.status(404).json({ 
          error: `Плата по адресу ${ip} не отвечает или недоступна` 
        });
      }

      console.log(`Плата найдена: ${boardInfo.name} (${boardInfo.type})`);

      // Создаем или обновляем запись станции в базе данных
      let station;
      try {
        // Пытаемся найти существующую станцию с таким IP
        const stations = await storage.getChargingStations();
        const existingStation = stations.find(s => s.ipAddress === ip);
        
        if (existingStation) {
          console.log(`Обновляем существующую станцию ID ${existingStation.id}`);
          // Обновляем существующую станцию
          station = await storage.updateChargingStation(existingStation.id, {
            displayName: boardInfo.name,
            technicalName: boardInfo.technicalName,
            type: boardInfo.type,
            status: 'online',
            ipAddress: ip,
            maxPower: boardInfo.maxPower,
            description: `ESP32 плата - ${boardInfo.type === 'master' ? 'Главный контроллер' : 'Контроллер станции'}`
          });
        } else {
          console.log('Создаем новую станцию в базе данных');
          // Создаем новую станцию
          station = await storage.createChargingStation({
            displayName: boardInfo.name,
            technicalName: boardInfo.technicalName,
            type: boardInfo.type,
            status: 'online',
            ipAddress: ip,
            maxPower: boardInfo.maxPower,
            description: `ESP32 плата - ${boardInfo.type === 'master' ? 'Главный контроллер' : 'Контроллер станции'}`,
            // Устанавливаем дефолтные значения для slave-плат
            ...(boardInfo.type === 'slave' && {
              carConnection: false,
              carChargingPermission: false,
              carError: false,
              masterOnline: true,
              masterChargingPermission: true,
              masterAvailablePower: 50,
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
            })
          });
        }
        
        console.log(`Станция сохранена с ID ${station.id}`);
        
        res.json({
          id: station.id,
          type: station.type,
          name: station.displayName,
          ip: station.ipAddress,
          status: station.status
        });
        
      } catch (storageError) {
        console.error('Ошибка сохранения станции:', storageError);
        res.status(500).json({ 
          error: "Ошибка сохранения данных станции" 
        });
      }

    } catch (error) {
      console.error('Ошибка подключения к ESP32:', error);
      res.status(500).json({ 
        error: "Ошибка подключения к плате ESP32" 
      });
    }
  });

  /**
   * POST /api/esp32/:id/sync
   * Синхронизация данных с платой ESP32
   */
  app.post("/api/esp32/:id/sync", async (req, res) => {
    try {
      const id = parseInt(req.params.id);
      
      if (isNaN(id)) {
        return res.status(400).json({ error: "Неверный ID станции" });
      }

      const station = await storage.getChargingStation(id);
      if (!station) {
        return res.status(404).json({ error: "Станция не найдена" });
      }

      if (!station.ipAddress) {
        return res.status(400).json({ error: "IP адрес станции не задан" });
      }

      console.log(`Синхронизация данных со станцией ${id} (${station.ipAddress})`);

      // Получаем актуальные данные с платы ESP32
      const boardData = await getDataFromESP32(station.ipAddress);
      
      if (!boardData) {
        return res.status(503).json({ 
          error: `Не удалось получить данные с платы ${station.ipAddress}` 
        });
      }

      // Обновляем данные станции на основе полученных с платы
      const updateData: any = {
        status: boardData.status || station.status,
        currentPower: boardData.current_power || station.currentPower
      };

      // Дополнительные поля для slave-плат
      if (station.type === 'slave' && boardData.slave_data) {
        Object.assign(updateData, {
          carConnection: boardData.slave_data.car_connected,
          carChargingPermission: boardData.slave_data.car_charging_permission,
          carError: boardData.slave_data.car_error,
          voltagePhase1: boardData.slave_data.voltage_phase1,
          voltagePhase2: boardData.slave_data.voltage_phase2,
          voltagePhase3: boardData.slave_data.voltage_phase3,
          currentPhase1: boardData.slave_data.current_phase1,
          currentPhase2: boardData.slave_data.current_phase2,
          currentPhase3: boardData.slave_data.current_phase3,
          chargerPower: boardData.slave_data.charger_power,
          singlePhaseConnection: boardData.slave_data.single_phase_connection,
          powerOverconsumption: boardData.slave_data.power_overconsumption,
          fixedPower: boardData.slave_data.fixed_power
        });
      }

      const updatedStation = await storage.updateChargingStation(id, updateData);
      
      console.log(`Данные станции ${id} успешно синхронизированы`);
      res.json(updatedStation);

    } catch (error) {
      console.error('Ошибка синхронизации с ESP32:', error);
      res.status(500).json({ 
        error: "Ошибка синхронизации данных с платой" 
      });
    }
  });

  // Создаем HTTP сервер с зарегистрированными маршрутами
  const httpServer = createServer(app);
  return httpServer;
}
