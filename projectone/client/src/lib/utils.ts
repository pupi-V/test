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
  switch (type) {
    case "master":
      return "station-master";
    case "slave":
      return "station-slave";
    case "undefined":
    default:
      return "text-gray-500";
  }
}

export function getTypeBgColor(type: string) {
  switch (type) {
    case "master":
      return "bg-station-master";
    case "slave":
      return "bg-station-slave";
    case "undefined":
    default:
      return "bg-gray-400";
  }
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
  switch (type) {
    case "master":
      return "MASTER";
    case "slave":
      return "SLAVE";
    case "undefined":
    default:
      return "НЕОПРЕДЕЛЕН";
  }
}

export function calculatePowerPercentage(current: number, max: number) {
  return max > 0 ? Math.round((current / max) * 100) : 0;
}
