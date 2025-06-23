import { chargingStations, type ChargingStation, type InsertChargingStation, type UpdateChargingStation } from "@shared/schema";
import fs from "fs/promises";
import path from "path";
import { watchFile } from "fs";

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
    this.setupFileWatcher();
  }

  /**
   * Создает папку для данных если она не существует
   */
  private async ensureDataDirectory() {
    const dataDir = path.dirname(this.dataPath);
    try {
      await fs.access(dataDir);
    } catch {
      // Папка не существует, создаем её
      await fs.mkdir(dataDir, { recursive: true });
    }
  }

  /**
   * Загружает данные из JSON файла в память
   */
  private async loadData() {
    try {
      await this.ensureDataDirectory();
      const data = await fs.readFile(this.dataPath, 'utf-8');
      const stationsArray: ChargingStation[] = JSON.parse(data);
      
      // Очищаем текущие данные и загружаем из файла
      this.stations.clear();
      stationsArray.forEach(station => {
        this.stations.set(station.id, station);
        if (station.id >= this.currentId) {
          this.currentId = station.id + 1;
        }
      });
      
      console.log(`Загружено ${stationsArray.length} станций из файла`);
    } catch (error) {
      console.log('Файл данных не найден, создаем новый');
      this.stations.clear();
      this.currentId = 1;
      await this.saveData();
    }
  }

  private setupFileWatcher() {
    watchFile(this.dataPath, { interval: 1000 }, (curr, prev) => {
      if (curr.mtime !== prev.mtime) {
        console.log('Обнаружены изменения в файле данных, перезагружаем...');
        this.loadData();
      }
    });
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
      status: "available",
      type: insertStation.type || "undefined",
      ipAddress: insertStation.ipAddress || null,
      description: insertStation.description || null,
      
      // Данные для slave-платы с начальными значениями
      carConnection: false,
      carChargingPermission: false,
      carError: false,
      masterOnline: false,
      masterChargingPermission: false,
      masterAvailablePower: 0,
      voltagePhase1: 0,
      voltagePhase2: 0,
      voltagePhase3: 0,
      currentPhase1: 0,
      currentPhase2: 0,
      currentPhase3: 0,
      chargerPower: 0,
      singlePhaseConnection: false,
      powerOverconsumption: false,
      fixedPower: false,
    };
    
    // Сохраняем в память и файл
    this.stations.set(id, station);
    await this.saveData();
    return station;
  }

  /**
   * Обновляет существующую станцию частичными данными
   */
  async updateChargingStation(id: number, updates: UpdateChargingStation): Promise<ChargingStation | undefined> {
    const existing = this.stations.get(id);
    if (!existing) {
      return undefined; // Станция не найдена
    }

    // Объединяем существующие данные с обновлениями
    const updated = { ...existing, ...updates };
    this.stations.set(id, updated);
    await this.saveData();
    return updated;
  }

  /**
   * Удаляет станцию по ID
   * Возвращает true если станция была удалена, false если не найдена
   */
  async deleteChargingStation(id: number): Promise<boolean> {
    const deleted = this.stations.delete(id);
    if (deleted) {
      // Сохраняем изменения в файл только если что-то было удалено
      await this.saveData();
    }
    return deleted;
  }
}

// Экспортируем единственный экземпляр хранилища для использования в приложении
export const storage = new JSONStorage();
