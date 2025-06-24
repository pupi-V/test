import { z } from "zod";

// Charging Station Schema
export const chargingStationSchema = z.object({
  id: z.number(),
  displayName: z.string(),
  technicalName: z.string(),
  type: z.enum(["master", "slave"]),
  status: z.enum(["available", "charging", "offline", "maintenance", "online"]),
  maxPower: z.number(),
  currentPower: z.number(),
  ipAddress: z.string().optional(),
  description: z.string().optional(),
  
  // Slave-specific fields
  carConnection: z.boolean(),
  carChargingPermission: z.boolean(),
  carError: z.boolean(),
  masterOnline: z.boolean(),
  masterChargingPermission: z.boolean(),
  masterAvailablePower: z.number(),
  
  // Electrical parameters
  voltagePhase1: z.number(),
  voltagePhase2: z.number(),
  voltagePhase3: z.number(),
  currentPhase1: z.number(),
  currentPhase2: z.number(),
  currentPhase3: z.number(),
  chargerPower: z.number(),
  
  // Additional parameters
  singlePhaseConnection: z.boolean(),
  powerOverconsumption: z.boolean(),
  fixedPower: z.boolean(),
});

export type ChargingStation = z.infer<typeof chargingStationSchema>;

// Insert schema for creating new stations
export const insertChargingStationSchema = chargingStationSchema.omit({ id: true });
export type InsertChargingStation = z.infer<typeof insertChargingStationSchema>;

// Update schema for updating existing stations
export const updateChargingStationSchema = chargingStationSchema.partial().omit({ id: true });
export type UpdateChargingStation = z.infer<typeof updateChargingStationSchema>;

// ESP32 Board Info Schema
export const esp32BoardSchema = z.object({
  id: z.string(),
  type: z.enum(["master", "slave"]),
  ip: z.string(),
  name: z.string(),
  status: z.enum(["online", "offline"]),
  lastSeen: z.string(),
});

export type ESP32Board = z.infer<typeof esp32BoardSchema>;