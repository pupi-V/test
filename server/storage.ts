// Импорты для работы с данными и файловой системой
import { chargingStations, type ChargingStation, type InsertChargingStation, type UpdateChargingStation } from "@shared/schema";
import fs from "fs/promises";
import path from "path";

/**
 * Интерфейс для работы с хранилищем зарядных станций
 * Определяет базовые операции CRUD (создание, чтение, обновление, удаление)
 */
export interface IStorage {
  // Получить список всех зарядных станций
  getChargingStations(): Promise<ChargingStation[]>;
  
  // Получить конкретную станцию по ID
  getChargingStation(id: number): Promise<ChargingStation | undefined>;
  
  // Создать новую зарядную станцию
  createChargingStation(station: InsertChargingStation): Promise<ChargingStation>;
  
  // Обновить существующую станцию
  updateChargingStation(id: number, updates: UpdateChargingStation): Promise<ChargingStation | undefined>;
  
  // Удалить станцию
  deleteChargingStation(id: number): Promise<boolean>;
}

/**
 * Реализация хранилища данных в JSON файле
 * Сохраняет данные о станциях в локальном JSON файле
 */
export class JSONStorage implements IStorage {
  private dataPath: string;              // Путь к файлу с данными
  private stations: Map<number, ChargingStation>; // Карта станций в памяти для быстрого доступа
  private currentId: number;             // Счетчик для генерации уникальных ID

  constructor() {
    // Устанавливаем путь к файлу данных в папке data/
    this.dataPath = path.resolve(process.cwd(), 'data', 'stations.json');
    this.stations = new Map();
    this.currentId = 1;
    // Загружаем существующие данные при инициализации
    this.loadData();
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
        // Обновляем счетчик ID чтобы избежать дублирования
        if (station.id >= this.currentId) {
          this.currentId = station.id + 1;
        }
      });
    } catch (error) {
      // Файл не существует или поврежден - начинаем с пустых данных
      this.stations.clear();
      this.currentId = 1;
      await this.saveData();
    }
  }

  /**
   * Сохраняет текущие данные из памяти в JSON файл
   */
  private async saveData() {
    await this.ensureDataDirectory();
    const stationsArray = Array.from(this.stations.values());
    await fs.writeFile(this.dataPath, JSON.stringify(stationsArray, null, 2));
  }

  /**
   * Возвращает список всех станций, отсортированный по ID
   */
  async getChargingStations(): Promise<ChargingStation[]> {
    return Array.from(this.stations.values()).sort((a, b) => a.id - b.id);
  }

  /**
   * Находит и возвращает станцию по её ID
   */
  async getChargingStation(id: number): Promise<ChargingStation | undefined> {
    return this.stations.get(id);
  }

  /**
   * Создает новую зарядную станцию с автогенерируемым ID
   */
  async createChargingStation(insertStation: InsertChargingStation): Promise<ChargingStation> {
    const id = this.currentId++; // Генерируем новый уникальный ID
    
    // Создаем полный объект станции с начальными значениями
    const station: ChargingStation = {
      ...insertStation,
      id,
      currentPower: 0,        // Начальная мощность = 0
      status: "available",    // Начальный статус = доступна
      ipAddress: insertStation.ipAddress || null,     // Обрабатываем undefined
      description: insertStation.description || null  // Обрабатываем undefined
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
