import { z } from "zod";

export const chargingStationSchema = z.object({
  id: z.number(),
  display_name: z.string(),
  technical_name: z.string(),
  type: z.enum(["AC", "DC", "Universal"]),
  status: z.enum(["available", "charging", "offline", "maintenance"]),
  max_power: z.number().positive(),
  current_power: z.number().min(0),
  available_power: z.number().min(0),
  car_connected: z.boolean(),
  charging_allowed: z.boolean(),
  charging_ready: z.boolean(),
  error_code: z.number().min(0),
  master_id: z.number().nullable(),
  voltage_l1: z.number().min(0),
  voltage_l2: z.number().min(0),
  voltage_l3: z.number().min(0),
  current_l1: z.number().min(0),
  current_l2: z.number().min(0),
  current_l3: z.number().min(0),
  power_l1: z.number().min(0),
  power_l2: z.number().min(0),
  power_l3: z.number().min(0),
  energy_delivered: z.number().min(0),
  session_time: z.number().min(0),
  temperature: z.number(),
  last_updated: z.string()
});

export type ChargingStation = z.infer<typeof chargingStationSchema>;

export const insertChargingStationSchema = chargingStationSchema.omit({ id: true });
export type InsertChargingStation = z.infer<typeof insertChargingStationSchema>;

export const updateChargingStationSchema = chargingStationSchema.partial().omit({ id: true });
export type UpdateChargingStation = z.infer<typeof updateChargingStationSchema>;

export const esp32BoardSchema = z.object({
  id: z.string(),
  type: z.enum(["master", "slave"]),
  ip: z.string(),
  name: z.string(),
  status: z.enum(["online", "offline"]),
  last_seen: z.string()
});

export type ESP32Board = z.infer<typeof esp32BoardSchema>;