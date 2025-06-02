import { useState, useEffect, useCallback, useRef } from "react";
import { useQuery, useMutation } from "@tanstack/react-query";
import { ChargingStation } from "@shared/schema";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Checkbox } from "@/components/ui/checkbox";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Button } from "@/components/ui/button";
import { ThemeToggle } from "@/components/theme-toggle";
import { BatteryCharging, Save, Wifi, RefreshCw } from "lucide-react";
import { useToast } from "@/hooks/use-toast";
import { queryClient, apiRequest } from "@/lib/queryClient";

interface SlaveControlProps {
  stationId: number;
}

/**
 * Страница управления slave-платой
 * Отображает интерфейс управления для slave станции с тремя основными секциями
 */
export default function SlaveControl({ stationId }: SlaveControlProps) {
  const { toast } = useToast();
  const autoSaveTimeoutRef = useRef<NodeJS.Timeout | null>(null);
  const hasInitialized = useRef(false);
  const justSaved = useRef(false);
  const [hasUnsavedChanges, setHasUnsavedChanges] = useState(false);
  
  // Состояние для всех полей slave-платы
  const [formData, setFormData] = useState({
    // Car секция
    carConnection: false,
    carChargingPermission: false,
    carError: false,
    
    // Master секция
    masterOnline: false,
    masterChargingPermission: false,
    masterAvailablePower: 0,
    
    // Charger секция - таблица мощности
    voltagePhase1: 0,
    voltagePhase2: 0,
    voltagePhase3: 0,
    currentPhase1: 0,
    currentPhase2: 0,
    currentPhase3: 0,
    chargerPower: 0,
    
    // Charger статус
    singlePhaseConnection: false,
    powerOverconsumption: false,
    fixedPower: false,
  });

  /**
   * Загружаем данные станции с автоматическим обновлением каждые 5 секунд
   * Такая же логика как в master-интерфейсе
   */
  const { data: station, isLoading, isFetching, refetch } = useQuery<ChargingStation>({
    queryKey: ['/api/stations', stationId],
    enabled: !!stationId,
    refetchInterval: 5000, // Автообновление каждые 5 секунд
  });

  /**
   * Мутация для обновления данных станции
   */
  const updateMutation = useMutation({
    mutationFn: async (data: Partial<ChargingStation>) => {
      return await apiRequest('PATCH', `/api/stations/${stationId}`, data);
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['/api/stations'] });
      setHasUnsavedChanges(false);
      toast({
        title: "Успешно",
        description: "Данные станции обновлены",
      });
    },
    onError: () => {
      toast({
        title: "Ошибка",
        description: "Не удалось обновить данные станции",
        variant: "destructive",
      });
    },
  });

  /**
   * Обновляем форму при загрузке данных станции
   * Не обновляем сразу после сохранения, чтобы предотвратить сброс изменений
   */
  useEffect(() => {
    if (station && !justSaved.current) {
      setFormData({
        carConnection: station.carConnection || false,
        carChargingPermission: station.carChargingPermission || false,
        carError: station.carError || false,
        masterOnline: station.masterOnline || false,
        masterChargingPermission: station.masterChargingPermission || false,
        masterAvailablePower: station.masterAvailablePower || 0,
        voltagePhase1: station.voltagePhase1 || 0,
        voltagePhase2: station.voltagePhase2 || 0,
        voltagePhase3: station.voltagePhase3 || 0,
        currentPhase1: station.currentPhase1 || 0,
        currentPhase2: station.currentPhase2 || 0,
        currentPhase3: station.currentPhase3 || 0,
        chargerPower: station.chargerPower || 0,
        singlePhaseConnection: station.singlePhaseConnection || false,
        powerOverconsumption: station.powerOverconsumption || false,
        fixedPower: station.fixedPower || false,
      });
    }
    
    // Сбрасываем флаг через короткое время после сохранения
    if (justSaved.current) {
      setTimeout(() => {
        justSaved.current = false;
      }, 1000);
    }
  }, [station]);

  /**
   * Обработчик ручного сохранения данных
   */
  const handleSave = () => {
    justSaved.current = true;
    updateMutation.mutate(formData);
    setHasUnsavedChanges(false);
  };

  /**
   * Принудительное обновление данных
   * Очищает кэш и загружает свежие данные с сервера
   */
  const handleRefresh = async () => {
    await refetch();
  };

  /**
   * Обработчик изменения checkbox
   */
  const handleCheckboxChange = (field: string, checked: boolean) => {
    setFormData(prev => ({ ...prev, [field]: checked }));
    setHasUnsavedChanges(true);
  };

  /**
   * Обработчик изменения числовых полей
   */
  const handleNumberChange = (field: string, value: string) => {
    const numValue = parseFloat(value) || 0;
    setFormData(prev => ({ ...prev, [field]: numValue }));
    setHasUnsavedChanges(true);
  };

  if (isLoading || !station) {
    return (
      <div className="min-h-screen bg-background flex items-center justify-center">
        <div className="text-center">
          <BatteryCharging className="mx-auto h-12 w-12 text-muted-foreground mb-4 animate-pulse" />
          <p className="text-muted-foreground">Загрузка данных станции...</p>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-background">
      {/* Header */}
      <header className="bg-card shadow-sm border-b border-border">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center h-16">
            <div className="flex items-center">
              <BatteryCharging className="text-primary text-2xl mr-3" />
              <div>
                <h1 className="text-xl font-medium text-foreground">
                  {station.displayName}
                </h1>
                <p className="text-sm text-muted-foreground">
                  {station.technicalName}
                </p>
              </div>
            </div>
            <div className="flex items-center space-x-4">
              {/* Индикатор статуса синхронизации */}
              <div className="flex items-center space-x-2 text-sm">
                <Wifi className={`h-4 w-4 ${
                  updateMutation.isPending 
                    ? 'text-yellow-500 animate-pulse' 
                    : hasUnsavedChanges 
                      ? 'text-orange-500' 
                      : 'text-green-500'
                }`} />
                <span className="text-muted-foreground">
                  {updateMutation.isPending 
                    ? 'Синхронизация...' 
                    : hasUnsavedChanges 
                      ? 'Автообновление приостановлено' 
                      : 'Синхронизировано'}
                </span>
              </div>
              
              <Button
                onClick={handleRefresh}
                disabled={isFetching}
                variant="outline"
                size="sm"
                className="flex items-center space-x-2"
              >
                <RefreshCw className={`h-4 w-4 ${isFetching ? 'animate-spin' : ''}`} />
                <span>{isFetching ? 'Обновление...' : 'Обновить'}</span>
              </Button>

              <Button
                onClick={handleSave}
                disabled={updateMutation.isPending || !hasUnsavedChanges}
                variant={hasUnsavedChanges ? "default" : "outline"}
                className="flex items-center space-x-2"
              >
                <Save className="h-4 w-4" />
                <span>Сохранить сейчас</span>
              </Button>
              <ThemeToggle />
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        {/* Информация о плате */}
        <div className="mb-8 text-center">
          <h1 className="text-3xl font-bold text-foreground mb-2">
            {station.displayName}
          </h1>
          <p className="text-lg text-muted-foreground">
            {station.technicalName}
          </p>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          
          {/* Верхний ряд: Car и Master */}
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 lg:col-span-2">
            {/* Car Section */}
            <Card>
              <CardHeader>
                <CardTitle className="text-lg text-foreground">Car</CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="flex items-center space-x-2">
                  <Checkbox
                    id="carConnection"
                    checked={formData.carConnection}
                    onCheckedChange={(checked) => 
                      handleCheckboxChange('carConnection', checked as boolean)
                    }
                  />
                  <Label htmlFor="carConnection" className="text-foreground">
                    Подключение
                  </Label>
                </div>
                
                <div className="flex items-center space-x-2">
                  <Checkbox
                    id="carChargingPermission"
                    checked={formData.carChargingPermission}
                    onCheckedChange={(checked) => 
                      handleCheckboxChange('carChargingPermission', checked as boolean)
                    }
                  />
                  <Label htmlFor="carChargingPermission" className="text-foreground">
                    Разрешение заряда
                  </Label>
                </div>
                
                <div className="flex items-center space-x-2">
                  <Checkbox
                    id="carError"
                    checked={formData.carError}
                    onCheckedChange={(checked) => 
                      handleCheckboxChange('carError', checked as boolean)
                    }
                  />
                  <Label htmlFor="carError" className="text-foreground">
                    Ошибка
                  </Label>
                </div>
              </CardContent>
            </Card>

            {/* Master Section */}
            <Card>
              <CardHeader>
                <CardTitle className="text-lg text-foreground">Master</CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="flex items-center space-x-2">
                  <Checkbox
                    id="masterOnline"
                    checked={formData.masterOnline}
                    onCheckedChange={(checked) => 
                      handleCheckboxChange('masterOnline', checked as boolean)
                    }
                  />
                  <Label htmlFor="masterOnline" className="text-foreground">
                    В сети
                  </Label>
                </div>
                
                <div className="flex items-center space-x-2">
                  <Checkbox
                    id="masterChargingPermission"
                    checked={formData.masterChargingPermission}
                    onCheckedChange={(checked) => 
                      handleCheckboxChange('masterChargingPermission', checked as boolean)
                    }
                  />
                  <Label htmlFor="masterChargingPermission" className="text-foreground">
                    Разрешение заряда
                  </Label>
                </div>
                
                <div className="space-y-2">
                  <Label htmlFor="masterAvailablePower" className="text-foreground">
                    Доступная мощность (кВт)
                  </Label>
                  <Input
                    id="masterAvailablePower"
                    type="number"
                    step="0.1"
                    value={formData.masterAvailablePower}
                    onChange={(e) => handleNumberChange('masterAvailablePower', e.target.value)}
                  />
                </div>
              </CardContent>
            </Card>
          </div>

          {/* Нижний ряд: Charger во всю ширину */}
          <Card className="lg:col-span-2">
            <CardHeader>
              <CardTitle className="text-lg text-foreground">Charger</CardTitle>
            </CardHeader>
            <CardContent className="space-y-6">
              {/* Power Table */}
              <div>
                <h4 className="text-sm font-medium text-foreground mb-3">Таблица мощности</h4>
                <div className="overflow-x-auto">
                  <table className="w-full text-sm">
                    <thead>
                      <tr className="border-b border-border">
                        <th className="text-left py-2 text-foreground">Фаза</th>
                        <th className="text-center py-2 text-foreground">1</th>
                        <th className="text-center py-2 text-foreground">2</th>
                        <th className="text-center py-2 text-foreground">3</th>
                      </tr>
                    </thead>
                    <tbody>
                      <tr className="border-b border-border">
                        <td className="py-2 text-foreground">Напряжение (В)</td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={formData.voltagePhase1}
                            onChange={(e) => handleNumberChange('voltagePhase1', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={formData.voltagePhase2}
                            onChange={(e) => handleNumberChange('voltagePhase2', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={formData.voltagePhase3}
                            onChange={(e) => handleNumberChange('voltagePhase3', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                      </tr>
                      <tr>
                        <td className="py-2 text-foreground">Ток (А)</td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={formData.currentPhase1}
                            onChange={(e) => handleNumberChange('currentPhase1', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={formData.currentPhase2}
                            onChange={(e) => handleNumberChange('currentPhase2', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={formData.currentPhase3}
                            onChange={(e) => handleNumberChange('currentPhase3', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                      </tr>
                    </tbody>
                  </table>
                </div>
              </div>

              {/* Power Field */}
              <div className="space-y-2">
                <Label htmlFor="chargerPower" className="text-foreground">
                  Мощность (кВт)
                </Label>
                <Input
                  id="chargerPower"
                  type="number"
                  step="0.1"
                  value={formData.chargerPower}
                  onChange={(e) => handleNumberChange('chargerPower', e.target.value)}
                />
              </div>

              {/* Status Checkboxes */}
              <div>
                <h4 className="text-sm font-medium text-foreground mb-3">Статус</h4>
                <div className="space-y-3">
                  <div className="flex items-center space-x-2">
                    <Checkbox
                      id="singlePhaseConnection"
                      checked={formData.singlePhaseConnection}
                      onCheckedChange={(checked) => 
                        handleCheckboxChange('singlePhaseConnection', checked as boolean)
                      }
                    />
                    <Label htmlFor="singlePhaseConnection" className="text-foreground">
                      Однофазное подключение
                    </Label>
                  </div>
                  
                  <div className="flex items-center space-x-2">
                    <Checkbox
                      id="powerOverconsumption"
                      checked={formData.powerOverconsumption}
                      onCheckedChange={(checked) => 
                        handleCheckboxChange('powerOverconsumption', checked as boolean)
                      }
                    />
                    <Label htmlFor="powerOverconsumption" className="text-foreground">
                      Перепотребление
                    </Label>
                  </div>
                  
                  <div className="flex items-center space-x-2">
                    <Checkbox
                      id="fixedPower"
                      checked={formData.fixedPower}
                      onCheckedChange={(checked) => 
                        handleCheckboxChange('fixedPower', checked as boolean)
                      }
                    />
                    <Label htmlFor="fixedPower" className="text-foreground">
                      Фиксированная мощность
                    </Label>
                  </div>
                </div>
              </div>
            </CardContent>
          </Card>
        </div>
      </main>
    </div>
  );
}