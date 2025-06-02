import { useState, useEffect } from "react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { ThemeToggle } from "@/components/theme-toggle";
import { BatteryCharging, Server, Cpu, Settings } from "lucide-react";
import { useLocation } from "wouter";
import { useQuery } from "@tanstack/react-query";
import type { ChargingStation } from "@shared/schema";

/**
 * Страница выбора типа платы
 * Позволяет выбрать тип платы для тестирования интерфейса
 */
export default function BoardSelector() {
  const [, setLocation] = useLocation();

  /**
   * Загружаем список станций для автоматического определения типа платы
   */
  const { data: stations } = useQuery<ChargingStation[]>({
    queryKey: ['/api/stations'],
  });

  /**
   * Автоматическое определение типа платы при загрузке
   * В реальной системе это будет определяться аппаратно
   */
  useEffect(() => {
    if (stations && stations.length > 0) {
      // Проверяем есть ли в системе master-станции
      const hasMasterStations = stations.some(station => station.type === "master");
      
      // Если нет master-станций, автоматически переходим на slave-интерфейс
      if (!hasMasterStations) {
        const firstSlaveStation = stations.find(station => station.type === "slave");
        if (firstSlaveStation) {
          setLocation(`/slave/${firstSlaveStation.id}`);
          return;
        }
      }
    }
  }, [stations, setLocation]);

  /**
   * Переход к dashboard для master-плат
   */
  const handleMasterBoard = () => {
    setLocation("/dashboard");
  };

  /**
   * Переход к интерфейсу slave-платы
   */
  const handleSlaveBoard = () => {
    // Находим первую доступную slave-станцию
    const slaveStation = stations?.find(station => station.type === "slave");
    if (slaveStation) {
      setLocation(`/slave/${slaveStation.id}`);
    } else {
      // Если нет slave-станций, используем первую доступную
      setLocation("/slave/1");
    }
  };

  return (
    <div className="min-h-screen bg-background">
      {/* Header */}
      <header className="bg-card shadow-sm border-b border-border">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center h-16">
            <div className="flex items-center">
              <Settings className="text-primary text-2xl mr-3" />
              <h1 className="text-xl font-medium text-foreground">
                Выбор типа платы
              </h1>
            </div>
            <ThemeToggle />
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
        <div className="text-center mb-12">
          <h2 className="text-3xl font-bold text-foreground mb-4">
            Система управления зарядными станциями
          </h2>
          <p className="text-lg text-muted-foreground">
            Выберите тип платы для загрузки соответствующего интерфейса
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
          {/* Master Board */}
          <Card className="cursor-pointer hover:shadow-lg transition-shadow duration-200 group">
            <CardHeader className="text-center pb-4">
              <div className="mx-auto w-16 h-16 bg-primary/10 rounded-full flex items-center justify-center mb-4 group-hover:bg-primary/20 transition-colors">
                <Server className="h-8 w-8 text-primary" />
              </div>
              <CardTitle className="text-xl text-foreground">
                Master плата
              </CardTitle>
            </CardHeader>
            <CardContent className="text-center">
              <p className="text-muted-foreground mb-6">
                Главная плата системы. Управляет множественными зарядными станциями, 
                отображает общий обзор и статистику всех подключенных устройств.
              </p>
              <ul className="text-sm text-muted-foreground mb-6 space-y-2">
                <li>• Мониторинг всех станций</li>
                <li>• Управление распределением мощности</li>
                <li>• Статистика и аналитика</li>
                <li>• Настройка системы</li>
              </ul>
              <Button 
                onClick={handleMasterBoard}
                className="w-full"
                size="lg"
              >
                <BatteryCharging className="mr-2 h-5 w-5" />
                Открыть Master интерфейс
              </Button>
            </CardContent>
          </Card>

          {/* Slave Board */}
          <Card className="cursor-pointer hover:shadow-lg transition-shadow duration-200 group">
            <CardHeader className="text-center pb-4">
              <div className="mx-auto w-16 h-16 bg-secondary/10 rounded-full flex items-center justify-center mb-4 group-hover:bg-secondary/20 transition-colors">
                <Cpu className="h-8 w-8 text-secondary-foreground" />
              </div>
              <CardTitle className="text-xl text-foreground">
                Slave плата
              </CardTitle>
            </CardHeader>
            <CardContent className="text-center">
              <p className="text-muted-foreground mb-6">
                Подчиненная плата отдельной зарядной станции. Управляет процессом зарядки 
                конкретного автомобиля и передает данные в главную систему.
              </p>
              <ul className="text-sm text-muted-foreground mb-6 space-y-2">
                <li>• Статус подключения автомобиля</li>
                <li>• Мониторинг параметров зарядки</li>
                <li>• Контроль мощности по фазам</li>
                <li>• Диагностика и ошибки</li>
              </ul>
              <Button 
                onClick={handleSlaveBoard}
                className="w-full"
                size="lg"
                variant="secondary"
              >
                <Cpu className="mr-2 h-5 w-5" />
                Открыть Slave интерфейс
              </Button>
            </CardContent>
          </Card>
        </div>

        {/* Info Section */}
        <div className="mt-12 text-center">
          <Card>
            <CardContent className="pt-6">
              <h3 className="text-lg font-medium text-foreground mb-4">
                Информация о системе
              </h3>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-6 text-sm text-muted-foreground">
                <div>
                  <h4 className="font-medium text-foreground mb-2">Master режим</h4>
                  <p>
                    Используется для централизованного управления несколькими зарядными станциями. 
                    Позволяет видеть общую картину системы и управлять распределением нагрузки.
                  </p>
                </div>
                <div>
                  <h4 className="font-medium text-foreground mb-2">Slave режим</h4>
                  <p>
                    Используется для управления отдельной зарядной станцией. 
                    Предоставляет детальную информацию о процессе зарядки и состоянии оборудования.
                  </p>
                </div>
              </div>
            </CardContent>
          </Card>
        </div>
      </main>
    </div>
  );
}