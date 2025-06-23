import { pgTable, text, serial, integer, real, boolean } from "drizzle-orm/pg-core";
import { createInsertSchema } from "drizzle-zod";
import { z } from "zod";

export const chargingStations = pgTable("charging_stations", {
  id: serial("id").primaryKey(),
  displayName: text("display_name").notNull(),
  technicalName: text("technical_name").notNull().unique(),
  type: text("type", { enum: ["master", "slave", "undefined"] }).notNull().default("undefined"),
  maxPower: real("max_power").notNull(),
  currentPower: real("current_power").notNull().default(0),
  status: text("status", { enum: ["charging", "available", "offline", "maintenance"] }).notNull().default("available"),
  ipAddress: text("ip_address"),
  description: text("description"),
  
  carConnection: boolean("car_connection").notNull().default(false),
  carChargingPermission: boolean("car_charging_permission").notNull().default(false),
  carError: boolean("car_error").notNull().default(false),
  
  masterOnline: boolean("master_online").notNull().default(false),
  masterChargingPermission: boolean("master_charging_permission").notNull().default(false),
  masterAvailablePower: real("master_available_power").notNull().default(0),
  
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
