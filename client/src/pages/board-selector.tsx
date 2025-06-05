import { useState, useEffect } from "react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { ThemeToggle } from "@/components/theme-toggle";
import { BatteryCharging, Wifi, AlertCircle, Loader2, RefreshCw } from "lucide-react";
import { useLocation } from "wouter";
import { useMutation, useQuery } from "@tanstack/react-query";
import { useToast } from "@/hooks/use-toast";
import { apiRequest } from "@/lib/queryClient";

interface ESP32Board {
  id: string;
  type: 'master' | 'slave';
  ip: string;
  name: string;
  status: 'online' | 'offline';
  lastSeen: string;
}

/**
 * Страница подключения к реальным платам ESP32
 * Сканирует сеть для поиска доступных плат и подключается к ним
 */
export default function BoardSelector() {
  const [, setLocation] = useLocation();
  const { toast } = useToast();
  const [isScanning, setIsScanning] = useState(false);
  const [manualIp, setManualIp] = useState("");
  const [foundBoards, setFoundBoards] = useState<ESP32Board[]>([]);

  /**
   * Загрузка списка подключенных станций для определения активных плат
   */
  const { data: stations, refetch: refetchStations } = useQuery<any[]>({
    queryKey: ['/api/stations'],
    refetchInterval: 5000 // Обновляем каждые 5 секунд
  });

  /**
   * Мутация для сканирования сети в поисках ESP32 плат
   */
  const scanMutation = useMutation({
    mutationFn: async () => {
      const response = await apiRequest('POST', '/api/esp32/scan', {});
      if (!response.ok) {
        throw new Error('Ошибка сканирования сети');
      }
      return await response.json();
    },
    onSuccess: (data: ESP32Board[]) => {
      setFoundBoards(data);
      toast({
        title: "Сканирование завершено",
        description: `Найдено плат: ${data.length}`,
      });
    },
    onError: (error: any) => {
      toast({
        title: "Ошибка сканирования",
        description: error.message,
        variant: "destructive",
      });
    },
  });

  /**
   * Мутация для подключения к конкретной плате ESP32
   */
  const connectMutation = useMutation({
    mutationFn: async (params: { ip: string; type?: 'master' | 'slave' }) => {
      const response = await apiRequest('POST', '/api/esp32/connect', params);
      if (!response.ok) {
        throw new Error(`Не удалось подключиться к плате по адресу ${params.ip}`);
      }
      return await response.json();
    },
    onSuccess: (data) => {
      toast({
        title: "Плата подключена",
        description: `Тип: ${data.type === 'master' ? 'Master' : 'Slave'} - ${data.name}`,
      });

      // Обновляем список станций
      refetchStations();

      // Перенаправляем на соответствующий интерфейс
      if (data.type === 'master') {
        setLocation('/dashboard');
      } else if (data.type === 'slave') {
        setLocation(`/slave/${data.id}`);
      }
    },
    onError: (error: any) => {
      toast({
        title: "Ошибка подключения",
        description: error.message,
        variant: "destructive",
      });
    },
  });

  /**
   * Запуск сканирования сети
   */
  const handleScan = () => {
    setIsScanning(true);
    scanMutation.mutate();
    
    // Останавливаем индикатор сканирования через 10 секунд
    setTimeout(() => setIsScanning(false), 10000);
  };

  /**
   * Подключение к плате по IP адресу
   */
  const handleConnectToBoard = (board: ESP32Board) => {
    connectMutation.mutate({ ip: board.ip, type: board.type });
  };

  /**
   * Ручное подключение по IP адресу
   */
  const handleManualConnect = () => {
    if (!manualIp.trim()) {
      toast({
        title: "Введите IP адрес",
        description: "Необходимо указать IP адрес платы ESP32",
        variant: "destructive",
      });
      return;
    }

    // Простая валидация IP адреса
    const ipPattern = /^(\d{1,3}\.){3}\d{1,3}$/;
    if (!ipPattern.test(manualIp)) {
      toast({
        title: "Неверный формат IP",
        description: "Введите корректный IP адрес (например: 192.168.1.100)",
        variant: "destructive",
      });
      return;
    }

    connectMutation.mutate({ ip: manualIp });
  };

  /**
   * Автоматическое сканирование при загрузке страницы
   */
  useEffect(() => {
    // Запускаем первое сканирование через 2 секунды после загрузки
    const timer = setTimeout(() => {
      handleScan();
    }, 2000);

    return () => clearTimeout(timer);
  }, []);

  return (
    <div className="min-h-screen bg-background">
      {/* Header */}
      <header className="bg-card border-b border-border">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center h-16">
            <div className="flex items-center">
              <BatteryCharging className="text-primary text-2xl mr-3" />
              <div>
                <h1 className="text-xl font-medium text-foreground">
                  Система зарядных станций
                </h1>
                <p className="text-sm text-muted-foreground">
                  Подключение к платам ESP32
                </p>
              </div>
            </div>
            <ThemeToggle />
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-6xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        <div className="text-center mb-8">
          <h2 className="text-3xl font-bold text-foreground mb-2">
            Подключение к платам ESP32
          </h2>
          <p className="text-lg text-muted-foreground">
            Автоматический поиск и подключение к реальным платам зарядных станций
          </p>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
          {/* Сканирование сети */}
          <Card>
            <CardHeader>
              <CardTitle className="flex items-center">
                <Wifi className="mr-2 h-5 w-5" />
                Сканирование сети
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-4">
              <p className="text-muted-foreground">
                Автоматический поиск плат ESP32 в локальной сети
              </p>
              
              <Button 
                onClick={handleScan}
                disabled={isScanning || scanMutation.isPending}
                className="w-full"
                size="lg"
              >
                {isScanning || scanMutation.isPending ? (
                  <>
                    <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                    Сканирование...
                  </>
                ) : (
                  <>
                    <RefreshCw className="mr-2 h-4 w-4" />
                    Сканировать сеть
                  </>
                )}
              </Button>

              {/* Список найденных плат */}
              {foundBoards.length > 0 && (
                <div className="space-y-2">
                  <h4 className="font-medium text-foreground">Найденные платы:</h4>
                  {foundBoards.map((board) => (
                    <div key={board.id} className="flex items-center justify-between p-3 border rounded-lg">
                      <div>
                        <div className="font-medium text-foreground">{board.name}</div>
                        <div className="text-sm text-muted-foreground">
                          {board.ip} • {board.type.toUpperCase()} • {board.status}
                        </div>
                      </div>
                      <Button
                        size="sm"
                        onClick={() => handleConnectToBoard(board)}
                        disabled={connectMutation.isPending}
                      >
                        Подключить
                      </Button>
                    </div>
                  ))}
                </div>
              )}
            </CardContent>
          </Card>

          {/* Ручное подключение */}
          <Card>
            <CardHeader>
              <CardTitle className="flex items-center">
                <AlertCircle className="mr-2 h-5 w-5" />
                Ручное подключение
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-4">
              <p className="text-muted-foreground">
                Подключение к плате по известному IP адресу
              </p>
              
              <div className="space-y-2">
                <Label htmlFor="manualIp" className="text-foreground">
                  IP адрес платы ESP32
                </Label>
                <Input
                  id="manualIp"
                  type="text"
                  placeholder="192.168.1.100"
                  value={manualIp}
                  onChange={(e) => setManualIp(e.target.value)}
                />
              </div>
              
              <Button 
                onClick={handleManualConnect}
                disabled={connectMutation.isPending || !manualIp.trim()}
                className="w-full"
                size="lg"
                variant="outline"
              >
                {connectMutation.isPending ? (
                  <>
                    <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                    Подключение...
                  </>
                ) : (
                  <>
                    <Wifi className="mr-2 h-4 w-4" />
                    Подключиться к плате
                  </>
                )}
              </Button>
            </CardContent>
          </Card>
        </div>

        {/* Информация о подключенных станциях */}
        {stations && Array.isArray(stations) && stations.length > 0 && (
          <Card className="mt-8">
            <CardHeader>
              <CardTitle>Активные станции</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
                {stations.map((station: any) => (
                  <div key={station.id} className="p-4 border rounded-lg">
                    <div className="font-medium text-foreground">{station.displayName}</div>
                    <div className="text-sm text-muted-foreground">
                      ID: {station.id} • {station.type.toUpperCase()}
                    </div>
                    <div className="text-sm text-muted-foreground">
                      IP: {station.ipAddress} • {station.status}
                    </div>
                    <Button
                      size="sm"
                      className="mt-2 w-full"
                      onClick={() => {
                        if (station.type === 'master') {
                          setLocation('/dashboard');
                        } else {
                          setLocation(`/slave/${station.id}`);
                        }
                      }}
                    >
                      Открыть интерфейс
                    </Button>
                  </div>
                ))}
              </div>
            </CardContent>
          </Card>
        )}

        {/* Информация о подключении */}
        <Card className="mt-8">
          <CardContent className="pt-6">
            <h3 className="text-lg font-medium text-foreground mb-4">
              Информация о подключении к ESP32
            </h3>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6 text-sm text-muted-foreground">
              <div>
                <h4 className="font-medium text-foreground mb-2">Master плата</h4>
                <ul className="space-y-1">
                  <li>• Главный контроллер системы</li>
                  <li>• Управляет множественными станциями</li>
                  <li>• Интерфейс обзора и статистики</li>
                  <li>• Распределение мощности</li>
                </ul>
              </div>
              <div>
                <h4 className="font-medium text-foreground mb-2">Slave плата</h4>
                <ul className="space-y-1">
                  <li>• Контроллер отдельной станции</li>
                  <li>• Управление процессом зарядки</li>
                  <li>• Мониторинг параметров</li>
                  <li>• Локальные настройки</li>
                </ul>
              </div>
            </div>
          </CardContent>
        </Card>
      </main>
    </div>
  );
}