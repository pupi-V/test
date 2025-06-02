// Импорты для React хуков и компонентов
import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { ChargingStation } from "@shared/schema";
import { Button } from "@/components/ui/button";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { Card, CardContent } from "@/components/ui/card";
import { BatteryCharging, Plus } from "lucide-react";
import StationCard from "@/components/station-card";
import AddStationModal from "@/components/add-station-modal";
import StationDetailsModal from "@/components/station-details-modal";
import DeleteConfirmationModal from "@/components/delete-confirmation-modal";

/**
 * Главная страница дашборда с плитками зарядных станций
 * Отображает все станции в виде карточек с возможностью фильтрации
 */
export default function Dashboard() {
  // Состояние для фильтров
  const [typeFilter, setTypeFilter] = useState<string>("all");     // Фильтр по типу станции
  const [statusFilter, setStatusFilter] = useState<string>("all"); // Фильтр по статусу
  
  // Состояние для модальных окон
  const [isAddModalOpen, setIsAddModalOpen] = useState(false);                          // Модалка добавления станции
  const [selectedStation, setSelectedStation] = useState<ChargingStation | null>(null); // Выбранная станция для детального просмотра
  const [stationToDelete, setStationToDelete] = useState<ChargingStation | null>(null); // Станция для удаления

  /**
   * Загружаем список всех зарядных станций
   * Автоматически обновляется каждые 5 секунд для отображения актуальных данных
   */
  const { data: stations = [], isLoading } = useQuery<ChargingStation[]>({
    queryKey: ["/api/stations"],
    refetchInterval: 5000, // Автообновление каждые 5 секунд
  });

  /**
   * Фильтруем станции по выбранным критериям
   */
  const filteredStations = stations.filter(station => {
    const typeMatch = typeFilter === "all" || station.type === typeFilter;
    const statusMatch = statusFilter === "all" || station.status === statusFilter;
    return typeMatch && statusMatch;
  });

  // Подсчет активных станций (со статусом "charging")
  const activeStations = stations.filter(s => s.status === "charging");

  /**
   * Обработчик клика по плитке станции - открывает детальный просмотр
   */
  const handleStationClick = (station: ChargingStation) => {
    setSelectedStation(station);
  };

  /**
   * Обработчик запроса на удаление станции
   */
  const handleDeleteRequest = (station: ChargingStation) => {
    setStationToDelete(station);
  };

  if (isLoading) {
    return (
      <div className="min-h-screen bg-gray-50">
        <div className="animate-pulse">
          <div className="bg-white h-16 shadow-sm border-b border-gray-200"></div>
          <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6">
              {[...Array(8)].map((_, i) => (
                <div key={i} className="bg-white rounded-xl shadow-sm h-64"></div>
              ))}
            </div>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gray-50">
      {/* Header */}
      <header className="bg-white shadow-sm border-b border-gray-200">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center h-16">
            <div className="flex items-center">
              <BatteryCharging className="text-blue-600 text-2xl mr-3" />
              <h1 className="text-xl font-medium text-gray-900">
                Система управления зарядными станциями
              </h1>
            </div>
            <div className="flex items-center space-x-4">
              <span className="text-sm text-gray-600">
                Станций: <span className="font-medium">{stations.length}</span>
              </span>
              <span className="text-sm text-gray-600">
                Активных: <span className="font-medium text-green-600">{activeStations.length}</span>
              </span>
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        {/* Controls Section */}
        <div className="flex justify-between items-center mb-8">
          <div>
            <h2 className="text-2xl font-medium text-gray-900 mb-2">Обзор станций</h2>
            <p className="text-gray-600">Управление и мониторинг зарядных станций</p>
          </div>
          <Button 
            onClick={() => setIsAddModalOpen(true)}
            className="bg-blue-600 hover:bg-blue-700 text-white font-medium"
          >
            <Plus className="mr-2 h-4 w-4" />
            Добавить станцию
          </Button>
        </div>

        {/* Filter Controls */}
        <Card className="mb-6">
          <CardContent className="pt-4">
            <div className="flex flex-wrap gap-4">
              <div className="flex items-center space-x-2">
                <label className="text-sm font-medium text-gray-600">Фильтр по типу:</label>
                <Select value={typeFilter} onValueChange={setTypeFilter}>
                  <SelectTrigger className="w-48">
                    <SelectValue />
                  </SelectTrigger>
                  <SelectContent>
                    <SelectItem value="all">Все станции</SelectItem>
                    <SelectItem value="master">Master станции</SelectItem>
                    <SelectItem value="slave">Slave станции</SelectItem>
                  </SelectContent>
                </Select>
              </div>
              <div className="flex items-center space-x-2">
                <label className="text-sm font-medium text-gray-600">Статус:</label>
                <Select value={statusFilter} onValueChange={setStatusFilter}>
                  <SelectTrigger className="w-48">
                    <SelectValue />
                  </SelectTrigger>
                  <SelectContent>
                    <SelectItem value="all">Все статусы</SelectItem>
                    <SelectItem value="charging">Заряжается</SelectItem>
                    <SelectItem value="available">Доступна</SelectItem>
                    <SelectItem value="offline">Не в сети</SelectItem>
                    <SelectItem value="maintenance">Обслуживание</SelectItem>
                  </SelectContent>
                </Select>
              </div>
            </div>
          </CardContent>
        </Card>

        {/* Station Grid */}
        {filteredStations.length === 0 ? (
          <Card className="text-center py-12">
            <CardContent>
              <BatteryCharging className="mx-auto h-12 w-12 text-gray-400 mb-4" />
              <h3 className="text-lg font-medium text-gray-900 mb-2">Нет станций</h3>
              <p className="text-gray-600 mb-4">
                {stations.length === 0 
                  ? "Начните с добавления первой зарядной станции"
                  : "Нет станций, соответствующих выбранным фильтрам"
                }
              </p>
              {stations.length === 0 && (
                <Button onClick={() => setIsAddModalOpen(true)}>
                  <Plus className="mr-2 h-4 w-4" />
                  Добавить станцию
                </Button>
              )}
            </CardContent>
          </Card>
        ) : (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6">
            {filteredStations.map((station) => (
              <StationCard 
                key={station.id} 
                station={station} 
                onClick={() => handleStationClick(station)}
                onDeleteRequest={() => handleDeleteRequest(station)}
              />
            ))}
          </div>
        )}
      </main>

      {/* Modals */}
      <AddStationModal 
        open={isAddModalOpen} 
        onOpenChange={setIsAddModalOpen} 
      />
      
      <StationDetailsModal 
        station={selectedStation}
        onClose={() => setSelectedStation(null)}
        onDeleteRequest={() => {
          if (selectedStation) {
            handleDeleteRequest(selectedStation);
          }
        }}
      />
      
      <DeleteConfirmationModal 
        station={stationToDelete}
        onClose={() => setStationToDelete(null)}
        onConfirm={() => {
          setStationToDelete(null);
          setSelectedStation(null);
        }}
      />
    </div>
  );
}
