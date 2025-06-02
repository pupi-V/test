import { pgTable, text, serial, integer, real } from "drizzle-orm/pg-core";
import { createInsertSchema } from "drizzle-zod";
import { z } from "zod";

export const chargingStations = pgTable("charging_stations", {
  id: serial("id").primaryKey(),
  displayName: text("display_name").notNull(),
  technicalName: text("technical_name").notNull().unique(),
  type: text("type", { enum: ["master", "slave"] }).notNull(),
  maxPower: real("max_power").notNull(),
  currentPower: real("current_power").notNull().default(0),
  status: text("status", { enum: ["charging", "available", "offline", "maintenance"] }).notNull().default("available"),
  ipAddress: text("ip_address"),
  description: text("description"),
});

export const insertChargingStationSchema = createInsertSchema(chargingStations).omit({
  id: true,
  currentPower: true,
});

export const updateChargingStationSchema = createInsertSchema(chargingStations).omit({
  id: true,
}).partial();

export type InsertChargingStation = z.infer<typeof insertChargingStationSchema>;
export type UpdateChargingStation = z.infer<typeof updateChargingStationSchema>;
export type ChargingStation = typeof chargingStations.$inferSelect;
