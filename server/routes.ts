// Импорты для работы с Express сервером и HTTP
import type { Express } from "express";
import { createServer, type Server } from "http";
import { storage } from "./storage";
import { insertChargingStationSchema, updateChargingStationSchema } from "@shared/schema";

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
   * POST /api/stations/:id/simulate
   * Симулирует изменение статуса и мощности станции
   * Используется для демонстрации динамических обновлений
   */
  app.post("/api/stations/:id/simulate", async (req, res) => {
    try {
      const id = parseInt(req.params.id);
      const station = await storage.getChargingStation(id);
      
      if (!station) {
        return res.status(404).json({ message: "Station not found" });
      }

      // Генерируем случайный статус для демонстрации
      const statuses = ["charging", "available", "offline", "maintenance"] as const;
      const newStatus = statuses[Math.floor(Math.random() * statuses.length)];
      
      // Если станция заряжает, генерируем случайную мощность
      // Иначе мощность = 0
      const newPower = newStatus === "charging" ? 
        Math.random() * station.maxPower : 0;

      // Обновляем станцию с новыми значениями
      const updated = await storage.updateChargingStation(id, {
        status: newStatus,
        currentPower: Math.round(newPower * 10) / 10 // Округляем до 1 знака после запятой
      });

      res.json(updated);
    } catch (error) {
      console.error("Error simulating station update:", error);
      res.status(500).json({ message: "Failed to simulate station update" });
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

  // Создаем HTTP сервер с зарегистрированными маршрутами
  const httpServer = createServer(app);
  return httpServer;
}
