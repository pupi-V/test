// Импорты для работы с базой данных и валидацией
import { pgTable, text, serial, integer, real, boolean } from "drizzle-orm/pg-core";
import { createInsertSchema } from "drizzle-zod";
import { z } from "zod";

/**
 * Схема таблицы зарядных станций
 * Определяет структуру данных для хранения информации о зарядных станциях
 */
export const chargingStations = pgTable("charging_stations", {
  // Уникальный идентификатор станции (автоинкремент)
  id: serial("id").primaryKey(),
  
  // Пользовательское имя станции (например "Станция А1")
  displayName: text("display_name").notNull(),
  
  // Техническое имя станции (например "CS-MASTER-001"), должно быть уникальным
  technicalName: text("technical_name").notNull().unique(),
  
  // Тип станции: master (главная), slave (подчиненная) или undefined (неопределенный)
  type: text("type", { enum: ["master", "slave", "undefined"] }).notNull().default("undefined"),
  
  // Максимальная мощность станции в киловаттах
  maxPower: real("max_power").notNull(),
  
  // Текущая потребляемая мощность в киловаттах (по умолчанию 0)
  currentPower: real("current_power").notNull().default(0),
  
  // Текущий статус станции с предопределенными значениями
  status: text("status", { enum: ["charging", "available", "offline", "maintenance"] }).notNull().default("available"),
  
  // IP-адрес станции для сетевого подключения (опционально)
  ipAddress: text("ip_address"),
  
  // Дополнительное описание станции (опционально)
  description: text("description"),
  
  // Данные для slave-платы
  // Car секция
  carConnection: boolean("car_connection").notNull().default(false),
  carChargingPermission: boolean("car_charging_permission").notNull().default(false),
  carError: boolean("car_error").notNull().default(false),
  
  // Master секция
  masterOnline: boolean("master_online").notNull().default(false),
  masterChargingPermission: boolean("master_charging_permission").notNull().default(false),
  masterAvailablePower: real("master_available_power").notNull().default(0),
  
  // Charger секция - таблица мощности
  voltagePhase1: real("voltage_phase1").notNull().default(0),
  voltagePhase2: real("voltage_phase2").notNull().default(0),
  voltagePhase3: real("voltage_phase3").notNull().default(0),
  currentPhase1: real("current_phase1").notNull().default(0),
  currentPhase2: real("current_phase2").notNull().default(0),
  currentPhase3: real("current_phase3").notNull().default(0),
  chargerPower: real("charger_power").notNull().default(0),
  
  // Charger статус
  singlePhaseConnection: boolean("single_phase_connection").notNull().default(false),
  powerOverconsumption: boolean("power_overconsumption").notNull().default(false),
  fixedPower: boolean("fixed_power").notNull().default(false),
});

/**
 * Схема валидации для создания новой станции
 * Исключает автогенерируемые поля (id, currentPower)
 */
export const insertChargingStationSchema = createInsertSchema(chargingStations).omit({
  id: true,          // ID генерируется автоматически
  currentPower: true, // Текущая мощность устанавливается системой
});

/**
 * Схема валидации для обновления существующей станции
 * Все поля опциональны, исключается ID и тип станции
 */
export const updateChargingStationSchema = createInsertSchema(chargingStations).omit({
  id: true,   // ID не может быть изменен
  type: true, // Тип станции не может быть изменен после создания
}).partial(); // Все поля становятся опциональными

// Типы TypeScript для использования в коде
export type InsertChargingStation = z.infer<typeof insertChargingStationSchema>;
export type UpdateChargingStation = z.infer<typeof updateChargingStationSchema>;
export type ChargingStation = typeof chargingStations.$inferSelect;
