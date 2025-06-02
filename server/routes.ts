import type { Express } from "express";
import { createServer, type Server } from "http";
import { storage } from "./storage";
import { insertChargingStationSchema, updateChargingStationSchema } from "@shared/schema";

export async function registerRoutes(app: Express): Promise<Server> {
  // Get all charging stations
  app.get("/api/stations", async (req, res) => {
    try {
      const stations = await storage.getChargingStations();
      res.json(stations);
    } catch (error) {
      res.status(500).json({ message: "Failed to fetch stations" });
    }
  });

  // Get single charging station
  app.get("/api/stations/:id", async (req, res) => {
    try {
      const id = parseInt(req.params.id);
      const station = await storage.getChargingStation(id);
      
      if (!station) {
        return res.status(404).json({ message: "Station not found" });
      }
      
      res.json(station);
    } catch (error) {
      res.status(500).json({ message: "Failed to fetch station" });
    }
  });

  // Create new charging station
  app.post("/api/stations", async (req, res) => {
    try {
      const validatedData = insertChargingStationSchema.parse(req.body);
      const station = await storage.createChargingStation(validatedData);
      res.status(201).json(station);
    } catch (error) {
      if (error.name === "ZodError") {
        return res.status(400).json({ message: "Invalid station data", errors: error.errors });
      }
      res.status(500).json({ message: "Failed to create station" });
    }
  });

  // Update charging station
  app.patch("/api/stations/:id", async (req, res) => {
    try {
      const id = parseInt(req.params.id);
      const validatedData = updateChargingStationSchema.parse(req.body);
      const station = await storage.updateChargingStation(id, validatedData);
      
      if (!station) {
        return res.status(404).json({ message: "Station not found" });
      }
      
      res.json(station);
    } catch (error) {
      if (error.name === "ZodError") {
        return res.status(400).json({ message: "Invalid station data", errors: error.errors });
      }
      res.status(500).json({ message: "Failed to update station" });
    }
  });

  // Delete charging station
  app.delete("/api/stations/:id", async (req, res) => {
    try {
      const id = parseInt(req.params.id);
      const deleted = await storage.deleteChargingStation(id);
      
      if (!deleted) {
        return res.status(404).json({ message: "Station not found" });
      }
      
      res.status(204).send();
    } catch (error) {
      res.status(500).json({ message: "Failed to delete station" });
    }
  });

  // Simulate status updates (for demo purposes)
  app.post("/api/stations/:id/simulate", async (req, res) => {
    try {
      const id = parseInt(req.params.id);
      const station = await storage.getChargingStation(id);
      
      if (!station) {
        return res.status(404).json({ message: "Station not found" });
      }

      // Simulate random status and power changes
      const statuses = ["charging", "available", "offline", "maintenance"] as const;
      const newStatus = statuses[Math.floor(Math.random() * statuses.length)];
      const newPower = newStatus === "charging" ? 
        Math.random() * station.maxPower : 0;

      const updated = await storage.updateChargingStation(id, {
        status: newStatus,
        currentPower: Math.round(newPower * 10) / 10
      });

      res.json(updated);
    } catch (error) {
      res.status(500).json({ message: "Failed to simulate station update" });
    }
  });

  const httpServer = createServer(app);
  return httpServer;
}
