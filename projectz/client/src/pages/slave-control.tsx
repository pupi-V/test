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

  // Состояние для строковых значений числовых полей во время редактирования
  const [inputValues, setInputValues] = useState({
    voltagePhase1: '',
    voltagePhase2: '',
    voltagePhase3: '',
    currentPhase1: '',
    currentPhase2: '',
    currentPhase3: '',
    chargerPower: '',
    masterAvailablePower: '',
  });

  /**
   * Загружаем данные станции с автоматическим обновлением каждые 5 секунд
   * Используем правильный API endpoint для получения конкретной станции
   */
  const { data: station, isLoading, isFetching, refetch } = useQuery<ChargingStation>({
    queryKey: [`/api/stations/${stationId}`],
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
      // Показываем разные уведомления для мгновенного и ручного сохранения
      if (justSaved.current) {
        toast({
          title: "Сохранено",
          description: "Настройка применена",
          duration: 2000,
        });
      } else {
        toast({
          title: "Успешно",
          description: "Все данные станции обновлены",
        });
      }
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
   * Всегда синхронизируем с актуальными данными с сервера
   */
  useEffect(() => {
    if (station && typeof station === 'object' && 'id' in station) {
      const stationData = station as ChargingStation;
      const newFormData = {
        carConnection: Boolean(stationData.carConnection),
        carChargingPermission: Boolean(stationData.carChargingPermission),
        carError: Boolean(stationData.carError),
        masterOnline: Boolean(stationData.masterOnline),
        masterChargingPermission: Boolean(stationData.masterChargingPermission),
        masterAvailablePower: Number(stationData.masterAvailablePower) || 0,
        voltagePhase1: Number(stationData.voltagePhase1) || 0,
        voltagePhase2: Number(stationData.voltagePhase2) || 0,
        voltagePhase3: Number(stationData.voltagePhase3) || 0,
        currentPhase1: Number(stationData.currentPhase1) || 0,
        currentPhase2: Number(stationData.currentPhase2) || 0,
        currentPhase3: Number(stationData.currentPhase3) || 0,
        chargerPower: Number(stationData.chargerPower) || 0,
        singlePhaseConnection: Boolean(stationData.singlePhaseConnection),
        powerOverconsumption: Boolean(stationData.powerOverconsumption),
        fixedPower: Boolean(stationData.fixedPower),
      };
      setFormData(newFormData);

      // Обновляем строковые значения для полей ввода
      setInputValues({
        voltagePhase1: String(Number(stationData.voltagePhase1) || 0),
        voltagePhase2: String(Number(stationData.voltagePhase2) || 0),
        voltagePhase3: String(Number(stationData.voltagePhase3) || 0),
        currentPhase1: String(Number(stationData.currentPhase1) || 0),
        currentPhase2: String(Number(stationData.currentPhase2) || 0),
        currentPhase3: String(Number(stationData.currentPhase3) || 0),
        chargerPower: String(Number(stationData.chargerPower) || 0),
        masterAvailablePower: String(Number(stationData.masterAvailablePower) || 0),
      });
    }
  }, [station]);

  /**
   * Обработчик ручного сохранения данных (для числовых полей)
   */
  const handleSave = () => {
    justSaved.current = false; // Для ручного сохранения показываем полное уведомление
    updateMutation.mutate(formData);
    setHasUnsavedChanges(false);
  };

  /**
   * Автоматическое сохранение с задержкой
   */
  const scheduleAutoSave = useCallback(() => {
    setHasUnsavedChanges(true);
    
    // Очищаем предыдущий таймер
    if (autoSaveTimeoutRef.current) {
      clearTimeout(autoSaveTimeoutRef.current);
    }
    
    // Устанавливаем новый таймер на 2 секунды
    autoSaveTimeoutRef.current = setTimeout(() => {
      justSaved.current = true;
      updateMutation.mutate(formData);
    }, 2000);
  }, [formData, updateMutation]);

  /**
   * Очищаем таймер при размонтировании компонента
   */
  useEffect(() => {
    return () => {
      if (autoSaveTimeoutRef.current) {
        clearTimeout(autoSaveTimeoutRef.current);
      }
    };
  }, []);

  /**
   * Принудительное обновление данных
   * Очищает кэш и загружает свежие данные с сервера
   */
  const handleRefresh = async () => {
    await refetch();
  };

  /**
   * Обработчик изменения checkbox с мгновенным сохранением
   */
  const handleCheckboxChange = (field: string, checked: boolean) => {
    // Обновляем локальное состояние
    setFormData(prev => ({ ...prev, [field]: checked }));
    
    // Мгновенное сохранение переключателей
    justSaved.current = true;
    updateMutation.mutate({ [field]: checked });
  };

  /**
   * Обработчик изменения числовых полей с автосохранением
   */
  const handleNumberChange = (field: string, value: string) => {
    // Обновляем строковое значение для отображения
    setInputValues(prev => ({ ...prev, [field]: value }));
    
    // Планируем автосохранение с конвертацией в число
    setHasUnsavedChanges(true);
    
    // Очищаем предыдущий таймер
    if (autoSaveTimeoutRef.current) {
      clearTimeout(autoSaveTimeoutRef.current);
    }
    
    // Устанавливаем новый таймер на 2 секунды
    autoSaveTimeoutRef.current = setTimeout(() => {
      // Конвертируем в число только при отправке
      const numValue = parseFloat(value) || 0;
      const dataToSend = { [field]: numValue };
      
      justSaved.current = true;
      updateMutation.mutate(dataToSend);
      
      // Обновляем локальное состояние после успешной отправки
      setFormData(prev => ({ ...prev, [field]: numValue }));
      setHasUnsavedChanges(false);
    }, 2000);
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
                  {(station as ChargingStation).displayName}
                </h1>
                <p className="text-sm text-muted-foreground">
                  {(station as ChargingStation).technicalName}
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
                    ? 'Сохранение...' 
                    : hasUnsavedChanges 
                      ? 'Автосохранение через 2 сек' 
                      : 'Все сохранено'}
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
            {(station as ChargingStation).displayName}
          </h1>
          <p className="text-lg text-muted-foreground">
            {(station as ChargingStation).technicalName}
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

            {/* Master Section - только для чтения, управляется через файлы */}
            <Card>
              <CardHeader>
                <CardTitle className="text-lg text-foreground">Master</CardTitle>
                <p className="text-sm text-muted-foreground">
                  Только для чтения - управляется master-платой
                </p>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="flex items-center space-x-2">
                  <Checkbox
                    id="masterOnline"
                    checked={formData.masterOnline}
                    disabled={true}
                  />
                  <Label htmlFor="masterOnline" className="text-muted-foreground">
                    В сети
                  </Label>
                </div>
                
                <div className="flex items-center space-x-2">
                  <Checkbox
                    id="masterChargingPermission"
                    checked={formData.masterChargingPermission}
                    disabled={true}
                  />
                  <Label htmlFor="masterChargingPermission" className="text-muted-foreground">
                    Разрешение заряда
                  </Label>
                </div>
                
                <div className="space-y-2">
                  <Label htmlFor="masterAvailablePower" className="text-muted-foreground">
                    Доступная мощность (кВт)
                  </Label>
                  <Input
                    id="masterAvailablePower"
                    type="number"
                    step="0.1"
                    value={inputValues.masterAvailablePower}
                    readOnly={true}
                    className="bg-muted text-muted-foreground cursor-not-allowed"
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
                            value={inputValues.voltagePhase1}
                            onChange={(e) => handleNumberChange('voltagePhase1', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={inputValues.voltagePhase2}
                            onChange={(e) => handleNumberChange('voltagePhase2', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={inputValues.voltagePhase3}
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
                            value={inputValues.currentPhase1}
                            onChange={(e) => handleNumberChange('currentPhase1', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={inputValues.currentPhase2}
                            onChange={(e) => handleNumberChange('currentPhase2', e.target.value)}
                            className="w-20 text-center"
                          />
                        </td>
                        <td className="py-2">
                          <Input
                            type="number"
                            step="0.1"
                            value={inputValues.currentPhase3}
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
                  value={inputValues.chargerPower}
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