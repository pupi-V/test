import { useState, useEffect } from "react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { ThemeToggle } from "@/components/theme-toggle";
import { BatteryCharging, Wifi, AlertCircle, Loader2, RefreshCw } from "lucide-react";
import { useLocation } from "wouter";
import { useMutation } from "@tanstack/react-query";
import { useToast } from "@/hooks/use-toast";
import { apiRequest } from "@/lib/queryClient";

/**
 * Страница ожидания подключения платы
 * Автоматически пытается определить подключенную плату и перенаправляет на соответствующий интерфейс
 */
export default function BoardWaiting() {
  const [, setLocation] = useLocation();
  const { toast } = useToast();
  const [isSearching, setIsSearching] = useState(true);
  const [searchAttempts, setSearchAttempts] = useState(0);
  const [lastError, setLastError] = useState<string>("");

  /**
   * Мутация для подключения платы
   */
  const connectMutation = useMutation({
    mutationFn: async (id: number) => {
      const response = await apiRequest('POST', '/api/board/connect', { boardId: id });
      if (!response.ok) {
        throw new Error(`Плата ${id} не найдена`);
      }
      return await response.json();
    },
    onSuccess: (data) => {
      toast({
        title: "Плата подключена",
        description: `Тип: ${data.type === 'master' ? 'Master' : 'Slave'} - ${data.displayName}`,
      });

      // Перенаправляем на соответствующий интерфейс
      if (data.type === 'master') {
        setLocation('/dashboard');
      } else if (data.type === 'slave') {
        setLocation(`/slave/${data.id}`);
      }
    },
    onError: (error: any) => {
      setLastError(error.message || "Ошибка подключения");
    },
  });

  /**
   * Функция поиска подключенных плат
   */
  const searchForBoards = async () => {
    setIsSearching(true);
    setSearchAttempts(prev => prev + 1);
    
    // Пробуем подключиться к платам с ID от 1 до 10
    const boardIds = Array.from({ length: 10 }, (_, i) => i + 1);
    
    for (const id of boardIds) {
      try {
        await connectMutation.mutateAsync(id);
        return; // Если подключение успешно, выходим из функции
      } catch (error) {
        // Продолжаем к следующей плате
        continue;
      }
    }
    
    // Если не удалось найти ни одной платы
    setIsSearching(false);
    setLastError("Подключенные платы не найдены");
  };

  /**
   * Автоматический поиск плат с задержкой при загрузке
   */
  useEffect(() => {
    // Показываем экран ожидания минимум 2 секунды
    const initialDelay = setTimeout(() => {
      searchForBoards();
    }, 2000);
    
    const interval = setInterval(() => {
      if (isSearching && searchAttempts > 0) {
        searchForBoards();
      }
    }, 5000);

    return () => {
      clearTimeout(initialDelay);
      clearInterval(interval);
    };
  }, []);

  /**
   * Ручной поиск плат
   */
  const handleManualSearch = () => {
    searchForBoards();
  };

  /**
   * Переход к ручному выбору типа платы
   */
  const handleManualSelection = () => {
    setLocation('/board-selector');
  };

  return (
    <div className="min-h-screen bg-background">
      {/* Header */}
      <header className="bg-card border-b border-border">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <div className="flex items-center space-x-3">
              <BatteryCharging className="h-8 w-8 text-primary" />
              <h1 className="text-2xl font-bold text-foreground">
                Система управления зарядными станциями
              </h1>
            </div>
            <ThemeToggle />
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
        <div className="text-center mb-8">
          <h2 className="text-3xl font-bold text-foreground mb-4">
            Ожидание подключения платы
          </h2>
          <p className="text-lg text-muted-foreground">
            Система автоматически ищет подключенные платы...
          </p>
        </div>

        <div className="flex justify-center">
          <Card className="w-full max-w-md">
            <CardHeader className="text-center pb-4">
              <div className="mx-auto w-20 h-20 bg-primary/10 rounded-full flex items-center justify-center mb-4">
                {isSearching ? (
                  <Loader2 className="h-10 w-10 text-primary animate-spin" />
                ) : (
                  <AlertCircle className="h-10 w-10 text-destructive" />
                )}
              </div>
              <CardTitle className="text-xl text-foreground">
                {isSearching ? "Поиск плат..." : "Платы не найдены"}
              </CardTitle>
            </CardHeader>
            <CardContent className="text-center space-y-4">
              {isSearching ? (
                <div>
                  <p className="text-muted-foreground mb-4">
                    {searchAttempts === 0 ? "Инициализация..." : `Попытка подключения #${searchAttempts}`}
                  </p>
                  <div className="flex items-center justify-center space-x-2 text-sm text-muted-foreground">
                    <Wifi className="h-4 w-4" />
                    <span>
                      {searchAttempts === 0 ? "Подготовка к поиску..." : "Сканирование портов..."}
                    </span>
                  </div>
                  {searchAttempts > 0 && (
                    <div className="mt-4 text-xs text-muted-foreground">
                      Проверяется ID платы: {searchAttempts <= 10 ? searchAttempts : "все доступные"}
                    </div>
                  )}
                </div>
              ) : (
                <div>
                  <p className="text-muted-foreground mb-4">
                    {lastError}
                  </p>
                  <p className="text-sm text-muted-foreground mb-6">
                    Убедитесь, что плата подключена и включена
                  </p>
                  
                  <div className="space-y-3">
                    <Button 
                      onClick={handleManualSearch}
                      className="w-full"
                      disabled={connectMutation.isPending}
                    >
                      <RefreshCw className="mr-2 h-4 w-4" />
                      Повторить поиск
                    </Button>
                    
                    <Button 
                      variant="outline"
                      onClick={handleManualSelection}
                      className="w-full"
                    >
                      Выбрать тип платы вручную
                    </Button>
                  </div>
                </div>
              )}
              
              {searchAttempts > 0 && (
                <div className="text-xs text-muted-foreground pt-4 border-t">
                  Попыток поиска: {searchAttempts}
                </div>
              )}
            </CardContent>
          </Card>
        </div>

        {/* Status Information */}
        <div className="mt-12 bg-card rounded-lg p-6 border border-border">
          <h3 className="text-lg font-semibold text-foreground mb-4">
            Информация о подключении
          </h3>
          <div className="grid md:grid-cols-2 gap-6">
            <div>
              <h4 className="font-medium text-foreground mb-2">Master плата</h4>
              <ul className="text-sm text-muted-foreground space-y-1">
                <li>• Управление всеми станциями</li>
                <li>• Мониторинг системы</li>
                <li>• Распределение мощности</li>
              </ul>
            </div>
            <div>
              <h4 className="font-medium text-foreground mb-2">Slave плата</h4>
              <ul className="text-sm text-muted-foreground space-y-1">
                <li>• Управление одной станцией</li>
                <li>• Локальные настройки</li>
                <li>• Статус подключения автомобиля</li>
              </ul>
            </div>
          </div>
        </div>
      </main>
    </div>
  );
}