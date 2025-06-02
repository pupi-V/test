import { type ClassValue, clsx } from "clsx";
import { twMerge } from "tailwind-merge";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

export function getStatusColor(status: string) {
  switch (status) {
    case "charging":
      return "status-charging";
    case "available":
      return "status-available";
    case "offline":
      return "status-offline";
    case "maintenance":
      return "status-maintenance";
    default:
      return "status-available";
  }
}

export function getStatusBgColor(status: string) {
  switch (status) {
    case "charging":
      return "bg-status-charging";
    case "available":
      return "bg-status-available";
    case "offline":
      return "bg-status-offline";
    case "maintenance":
      return "bg-status-maintenance";
    default:
      return "bg-status-available";
  }
}

export function getTypeColor(type: string) {
  return type === "master" ? "station-master" : "station-slave";
}

export function getTypeBgColor(type: string) {
  return type === "master" ? "bg-station-master" : "bg-station-slave";
}

export function getStatusLabel(status: string) {
  switch (status) {
    case "charging":
      return "Заряжается";
    case "available":
      return "Доступна";
    case "offline":
      return "Не в сети";
    case "maintenance":
      return "Обслуживание";
    default:
      return "Неизвестно";
  }
}

export function getTypeLabel(type: string) {
  return type === "master" ? "MASTER" : "SLAVE";
}

export function calculatePowerPercentage(current: number, max: number) {
  return max > 0 ? Math.round((current / max) * 100) : 0;
}
