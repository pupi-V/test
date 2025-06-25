import { ChargingStation } from "../types/schema";
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Edit, Trash2 } from "lucide-react";
import { getStatusColor, getStatusBgColor, getTypeColor, getTypeBgColor, getStatusLabel, getTypeLabel, calculatePowerPercentage } from "@/lib/utils";

interface StationCardProps {
  station: ChargingStation;
  onClick: () => void;
  onDeleteRequest: () => void;
}

export default function StationCard({ station, onClick, onDeleteRequest }: StationCardProps) {
  const powerPercentage = calculatePowerPercentage(station.currentPower, station.maxPower);

  const handleEdit = (e: React.MouseEvent) => {
    e.stopPropagation();
    onClick();
  };

  const handleDelete = (e: React.MouseEvent) => {
    e.stopPropagation();
    onDeleteRequest();
  };

  return (
    <Card 
      className="cursor-pointer hover:shadow-md transition-shadow duration-200 group"
      onClick={onClick}
    >
      <CardContent className="p-6">
        <div className="flex justify-between items-start mb-4">
          <div className="flex items-center">
            <div className={`w-3 h-3 rounded-full mr-2 ${getTypeBgColor(station.type)}`}></div>
            <span className={`text-xs font-medium uppercase tracking-wider ${getTypeColor(station.type)}`}>
              {getTypeLabel(station.type)}
            </span>
          </div>
          <div className="flex space-x-2">
            <Button
              variant="ghost"
              size="sm"
              className="text-muted-foreground hover:text-primary p-1 h-auto"
              onClick={handleEdit}
            >
              <Edit className="h-4 w-4" />
            </Button>
            <Button
              variant="ghost"
              size="sm"
              className="text-muted-foreground hover:text-destructive p-1 h-auto"
              onClick={handleDelete}
            >
              <Trash2 className="h-4 w-4" />
            </Button>
          </div>
        </div>
        
        <h3 className="text-lg font-medium text-foreground mb-1 group-hover:text-primary transition-colors">
          {station.displayName}
        </h3>
        <p className="text-sm text-muted-foreground mb-4">
          {station.technicalName}
        </p>
        
        <div className="space-y-3">
          <div className="flex justify-between items-center">
            <span className="text-sm text-muted-foreground">Макс. мощность:</span>
            <span className="text-sm font-medium text-foreground">{station.maxPower} кВт</span>
          </div>
          
          <div className="flex justify-between items-center">
            <span className="text-sm text-muted-foreground">Статус:</span>
            <div className="flex items-center">
              <div className={`w-2 h-2 rounded-full mr-2 ${getStatusBgColor(station.status)}`}></div>
              <span className={`text-sm font-medium ${getStatusColor(station.status)}`}>
                {getStatusLabel(station.status)}
              </span>
            </div>
          </div>
          
          <div className="flex justify-between items-center">
            <span className="text-sm text-muted-foreground">Текущая мощность:</span>
            <span className={`text-sm font-medium ${station.status === "charging" ? "text-green-600 dark:text-green-400" : "text-muted-foreground"}`}>
              {station.status === "offline" ? "- кВт" : `${station.currentPower} кВт`}
            </span>
          </div>
          
          {/* Power Usage Bar */}
          <div className="w-full bg-muted rounded-full h-2 mt-3">
            <div 
              className={`h-2 rounded-full transition-all duration-300 ${getStatusBgColor(station.status)}`}
              style={{ width: `${powerPercentage}%` }}
            ></div>
          </div>
        </div>
      </CardContent>
    </Card>
  );
}
