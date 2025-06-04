import { useState, useEffect } from "react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { ThemeToggle } from "@/components/theme-toggle";
import { BatteryCharging, Wifi, AlertCircle, Loader2 } from "lucide-react";
import { useLocation } from "wouter";
import { useMutation } from "@tanstack/react-query";
import { useToast } from "@/hooks/use-toast";
import { apiRequest } from "@/lib/queryClient";

/**
 * Страница автоматического подключения платы
 * Определяет тип платы через JSON запрос и перенаправляет на соответствующий интерфейс
 */
export default function BoardConnect() {
  const [, setLocation] = useLocation();
  const { toast } = useToast();
  const [boardId, setBoardId] = useState("");
  const [isAutoConnecting, setIsAutoConnecting] = useState(true);

  /**
   * Мутация для подключения платы
   */
  const connectMutation = useMutation({
    mutationFn: async (id: number) => {
      const response = await apiRequest('POST', '/api/board/connect', { boardId: id });
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
      toast({
        title: "Ошибка подключения",
        description: error.message || "Не удалось подключиться к плате",
        variant: "destructive",
      });
      setIsAutoConnecting(false);
    },
  });

  /**
   * Автоматическое подключение при загрузке страницы
   * Пытается подключиться к платам с ID 1, 2, 3
   */
  useEffect(() => {
    const autoConnect = async () => {
      // Пробуем подключиться к платам по порядку
      const boardIds = [1, 2, 3];
      
      for (const id of boardIds) {
        try {
          await connectMutation.mutateAsync(id);
          return; // Если подключение успешно, выходим из цикла
        } catch (error) {
          // Продолжаем к следующей плате
          continue;
        }
      }
      
      // Если не удалось подключиться ни к одной плате
      setIsAutoConnecting(false);
      toast({
        title: "Автоподключение не удалось",
        description: "Введите ID платы вручную",
        variant: "destructive",
      });
    };

    autoConnect();
  }, []);

  /**
   * Ручное подключение к плате
   */
  const handleManualConnect = () => {
    const id = parseInt(boardId);
    if (isNaN(id) || id <= 0) {
      toast({
        title: "Неверный ID",
        description: "Введите корректный ID платы",
        variant: "destructive",
      });
      return;
    }

    connectMutation.mutate(id);
  };

  /**
   * Переход к выбору типа платы вручную
   */
  const handleManualSelection = () => {
    setLocation('/board-selector');
  };

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
                  Подключение платы
                </p>
              </div>
            </div>
            <ThemeToggle />
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-2xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
        <div className="text-center mb-8">
          <h1 className="text-3xl font-bold text-foreground mb-2">
            Подключение к плате
          </h1>
          <p className="text-lg text-muted-foreground">
            Автоматическое определение типа платы и перенаправление на соответствующий интерфейс
          </p>
        </div>

        {isAutoConnecting ? (
          /* Автоматическое подключение */
          <Card>
            <CardHeader className="text-center">
              <div className="mx-auto w-16 h-16 bg-primary/10 rounded-full flex items-center justify-center mb-4">
                <Loader2 className="h-8 w-8 text-primary animate-spin" />
              </div>
              <CardTitle className="text-xl text-foreground">
                Подключение к плате...
              </CardTitle>
            </CardHeader>
            <CardContent className="text-center space-y-4">
              <p className="text-muted-foreground">
                Выполняется автоматическое определение типа платы
              </p>
              <div className="flex items-center justify-center space-x-2">
                <Wifi className="h-4 w-4 text-primary" />
                <span className="text-sm text-muted-foreground">
                  Поиск доступных плат...
                </span>
              </div>
              {connectMutation.isPending && (
                <div className="flex items-center justify-center space-x-2 mt-4">
                  <div className="h-2 w-2 bg-primary rounded-full animate-bounce" />
                  <div className="h-2 w-2 bg-primary rounded-full animate-bounce" style={{ animationDelay: '0.1s' }} />
                  <div className="h-2 w-2 bg-primary rounded-full animate-bounce" style={{ animationDelay: '0.2s' }} />
                </div>
              )}
            </CardContent>
          </Card>
        ) : (
          /* Ручное подключение */
          <div className="space-y-6">
            <Card>
              <CardHeader className="text-center">
                <div className="mx-auto w-16 h-16 bg-orange-100 dark:bg-orange-900/20 rounded-full flex items-center justify-center mb-4">
                  <AlertCircle className="h-8 w-8 text-orange-600 dark:text-orange-400" />
                </div>
                <CardTitle className="text-xl text-foreground">
                  Ручное подключение
                </CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                <p className="text-center text-muted-foreground mb-6">
                  Автоматическое подключение не удалось. Введите ID платы вручную.
                </p>
                
                <div className="space-y-2">
                  <Label htmlFor="boardId" className="text-foreground">
                    ID платы
                  </Label>
                  <Input
                    id="boardId"
                    type="number"
                    placeholder="Введите ID платы (например: 1, 2, 3)"
                    value={boardId}
                    onChange={(e) => setBoardId(e.target.value)}
                    className="text-center"
                  />
                </div>
                
                <Button 
                  onClick={handleManualConnect}
                  disabled={connectMutation.isPending || !boardId}
                  className="w-full"
                  size="lg"
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

            <Card>
              <CardContent className="pt-6">
                <div className="text-center">
                  <p className="text-sm text-muted-foreground mb-4">
                    Или выберите тип платы вручную
                  </p>
                  <Button 
                    variant="outline" 
                    onClick={handleManualSelection}
                    className="w-full"
                  >
                    Выбрать тип платы вручную
                  </Button>
                </div>
              </CardContent>
            </Card>
          </div>
        )}
      </main>
    </div>
  );
}