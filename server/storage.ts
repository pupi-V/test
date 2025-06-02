import { chargingStations, type ChargingStation, type InsertChargingStation, type UpdateChargingStation } from "@shared/schema";
import fs from "fs/promises";
import path from "path";

export interface IStorage {
  getChargingStations(): Promise<ChargingStation[]>;
  getChargingStation(id: number): Promise<ChargingStation | undefined>;
  createChargingStation(station: InsertChargingStation): Promise<ChargingStation>;
  updateChargingStation(id: number, updates: UpdateChargingStation): Promise<ChargingStation | undefined>;
  deleteChargingStation(id: number): Promise<boolean>;
}

export class JSONStorage implements IStorage {
  private dataPath: string;
  private stations: Map<number, ChargingStation>;
  private currentId: number;

  constructor() {
    this.dataPath = path.resolve(process.cwd(), 'data', 'stations.json');
    this.stations = new Map();
    this.currentId = 1;
    this.loadData();
  }

  private async ensureDataDirectory() {
    const dataDir = path.dirname(this.dataPath);
    try {
      await fs.access(dataDir);
    } catch {
      await fs.mkdir(dataDir, { recursive: true });
    }
  }

  private async loadData() {
    try {
      await this.ensureDataDirectory();
      const data = await fs.readFile(this.dataPath, 'utf-8');
      const stationsArray: ChargingStation[] = JSON.parse(data);
      
      this.stations.clear();
      stationsArray.forEach(station => {
        this.stations.set(station.id, station);
        if (station.id >= this.currentId) {
          this.currentId = station.id + 1;
        }
      });
    } catch (error) {
      // File doesn't exist or is invalid, start with empty data
      this.stations.clear();
      this.currentId = 1;
      await this.saveData();
    }
  }

  private async saveData() {
    await this.ensureDataDirectory();
    const stationsArray = Array.from(this.stations.values());
    await fs.writeFile(this.dataPath, JSON.stringify(stationsArray, null, 2));
  }

  async getChargingStations(): Promise<ChargingStation[]> {
    return Array.from(this.stations.values()).sort((a, b) => a.id - b.id);
  }

  async getChargingStation(id: number): Promise<ChargingStation | undefined> {
    return this.stations.get(id);
  }

  async createChargingStation(insertStation: InsertChargingStation): Promise<ChargingStation> {
    const id = this.currentId++;
    const station: ChargingStation = {
      ...insertStation,
      id,
      currentPower: 0,
      status: "available"
    };
    
    this.stations.set(id, station);
    await this.saveData();
    return station;
  }

  async updateChargingStation(id: number, updates: UpdateChargingStation): Promise<ChargingStation | undefined> {
    const existing = this.stations.get(id);
    if (!existing) {
      return undefined;
    }

    const updated = { ...existing, ...updates };
    this.stations.set(id, updated);
    await this.saveData();
    return updated;
  }

  async deleteChargingStation(id: number): Promise<boolean> {
    const deleted = this.stations.delete(id);
    if (deleted) {
      await this.saveData();
    }
    return deleted;
  }
}

export const storage = new JSONStorage();
