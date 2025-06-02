import { useEffect } from "react";
import { useMutation, useQueryClient } from "@tanstack/react-query";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import { updateChargingStationSchema, type UpdateChargingStation, type ChargingStation } from "@shared/schema";
import { apiRequest } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import {
  Form,
  FormControl,
  FormField,
  FormItem,
  FormLabel,
  FormMessage,
} from "@/components/ui/form";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { Textarea } from "@/components/ui/textarea";
import { Trash2 } from "lucide-react";
import { getStatusColor, getStatusBgColor, getTypeColor, getTypeBgColor, getStatusLabel, getTypeLabel, calculatePowerPercentage } from "@/lib/utils";

interface StationDetailsModalProps {
  station: ChargingStation | null;
  onClose: () => void;
  onDeleteRequest: () => void;
}

export default function StationDetailsModal({ station, onClose, onDeleteRequest }: StationDetailsModalProps) {
  const { toast } = useToast();
  const queryClient = useQueryClient();

  const form = useForm<UpdateChargingStation>({
    resolver: zodResolver(updateChargingStationSchema),
    defaultValues: {
      displayName: "",
      technicalName: "",
      type: "master",
      maxPower: 22,
      status: "available",
      ipAddress: "",
      description: "",
    },
  });

  useEffect(() => {
    if (station) {
      form.reset({
        displayName: station.displayName,
        technicalName: station.technicalName,
        type: station.type,
        maxPower: station.maxPower,
        status: station.status,
        ipAddress: station.ipAddress || "",
        description: station.description || "",
      });
    }
  }, [station, form]);

  const updateStationMutation = useMutation({
    mutationFn: async (data: UpdateChargingStation) => {
      if (!station) throw new Error("No station selected");
      const response = await apiRequest("PATCH", `/api/stations/${station.id}`, data);
      return response.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/stations"] });
      toast({
        title: "Станция обновлена",
        description: "Настройки станции успешно сохранены",
      });
      onClose();
    },
    onError: (error: any) => {
      toast({
        title: "Ошибка",
        description: error.message || "Не удалось обновить станцию",
        variant: "destructive",
      });
    },
  });

  const simulateUpdateMutation = useMutation({
    mutationFn: async () => {
      if (!station) throw new Error("No station selected");
      const response = await apiRequest("POST", `/api/stations/${station.id}/simulate`, {});
      return response.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/stations"] });
      toast({
        title: "Статус обновлен",
        description: "Статус станции был случайно изменен для демонстрации",
      });
    },
    onError: (error: any) => {
      toast({
        title: "Ошибка",
        description: error.message || "Не удалось обновить статус",
        variant: "destructive",
      });
    },
  });

  const onSubmit = (data: UpdateChargingStation) => {
    updateStationMutation.mutate(data);
  };

  if (!station) return null;

  const powerPercentage = calculatePowerPercentage(station.currentPower, station.maxPower);

  return (
    <Dialog open={!!station} onOpenChange={() => onClose()}>
      <DialogContent className="sm:max-w-2xl max-h-[90vh] overflow-y-auto">
        <DialogHeader>
          <DialogTitle>Детали станции</DialogTitle>
        </DialogHeader>
        
        <div className="space-y-6">
          {/* Station Header */}
          <div className="flex items-center justify-between">
            <div>
              <div className="flex items-center mb-2">
                <div className={`w-4 h-4 rounded-full mr-3 ${getTypeBgColor(station.type)}`}></div>
                <span className={`text-sm font-medium uppercase tracking-wider ${getTypeColor(station.type)}`}>
                  {getTypeLabel(station.type)}
                </span>
              </div>
              <h4 className="text-xl font-medium text-gray-900">{station.displayName}</h4>
              <p className="text-gray-600">{station.technicalName}</p>
            </div>
            <div className="text-right">
              <div className="flex items-center justify-end mb-1">
                <div className={`w-3 h-3 rounded-full mr-2 ${getStatusBgColor(station.status)}`}></div>
                <span className={`text-lg font-medium ${getStatusColor(station.status)}`}>
                  {getStatusLabel(station.status)}
                </span>
              </div>
              <p className="text-sm text-gray-600">Последнее обновление: 2 мин назад</p>
            </div>
          </div>

          {/* Power Statistics */}
          <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
            <div className="bg-gray-50 rounded-lg p-4">
              <h5 className="text-sm font-medium text-gray-600 mb-1">Максимальная мощность</h5>
              <p className="text-2xl font-bold text-gray-900">{station.maxPower} кВт</p>
            </div>
            <div className="bg-gray-50 rounded-lg p-4">
              <h5 className="text-sm font-medium text-gray-600 mb-1">Текущая мощность</h5>
              <p className={`text-2xl font-bold ${station.status === "charging" ? "text-green-600" : "text-gray-900"}`}>
                {station.status === "offline" ? "- кВт" : `${station.currentPower} кВт`}
              </p>
            </div>
            <div className="bg-gray-50 rounded-lg p-4">
              <h5 className="text-sm font-medium text-gray-600 mb-1">Эффективность</h5>
              <p className="text-2xl font-bold text-gray-900">{powerPercentage}%</p>
            </div>
          </div>

          {/* Demo Button */}
          <div className="flex justify-center">
            <Button 
              onClick={() => simulateUpdateMutation.mutate()}
              disabled={simulateUpdateMutation.isPending}
              variant="outline"
              className="bg-yellow-50 border-yellow-200 text-yellow-800 hover:bg-yellow-100"
            >
              {simulateUpdateMutation.isPending ? "Обновление..." : "🎲 Симулировать изменение статуса"}
            </Button>
          </div>

          {/* Configuration Form */}
          <Form {...form}>
            <form onSubmit={form.handleSubmit(onSubmit)} className="space-y-4">
              <h5 className="text-lg font-medium text-gray-900 border-b border-gray-200 pb-2">
                Настройки станции
              </h5>
              
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <FormField
                  control={form.control}
                  name="displayName"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel>Отображаемое имя</FormLabel>
                      <FormControl>
                        <Input {...field} />
                      </FormControl>
                      <FormMessage />
                    </FormItem>
                  )}
                />
                
                <FormField
                  control={form.control}
                  name="technicalName"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel>Техническое имя</FormLabel>
                      <FormControl>
                        <Input {...field} />
                      </FormControl>
                      <FormMessage />
                    </FormItem>
                  )}
                />
              </div>
              
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <FormField
                  control={form.control}
                  name="type"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel>Тип станции</FormLabel>
                      <Select onValueChange={field.onChange} value={field.value}>
                        <FormControl>
                          <SelectTrigger>
                            <SelectValue />
                          </SelectTrigger>
                        </FormControl>
                        <SelectContent>
                          <SelectItem value="master">Master станция</SelectItem>
                          <SelectItem value="slave">Slave станция</SelectItem>
                        </SelectContent>
                      </Select>
                      <FormMessage />
                    </FormItem>
                  )}
                />
                
                <FormField
                  control={form.control}
                  name="maxPower"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel>Максимальная мощность (кВт)</FormLabel>
                      <FormControl>
                        <Input 
                          type="number" 
                          step="0.1" 
                          min="1" 
                          max="350"
                          {...field}
                          onChange={(e) => field.onChange(parseFloat(e.target.value) || 0)}
                        />
                      </FormControl>
                      <FormMessage />
                    </FormItem>
                  )}
                />
              </div>

              <FormField
                control={form.control}
                name="ipAddress"
                render={({ field }) => (
                  <FormItem>
                    <FormLabel>IP адрес</FormLabel>
                    <FormControl>
                      <Input {...field} />
                    </FormControl>
                    <FormMessage />
                  </FormItem>
                )}
              />

              <FormField
                control={form.control}
                name="description"
                render={({ field }) => (
                  <FormItem>
                    <FormLabel>Описание</FormLabel>
                    <FormControl>
                      <Textarea 
                        rows={3} 
                        placeholder="Дополнительная информация о станции..."
                        {...field} 
                      />
                    </FormControl>
                    <FormMessage />
                  </FormItem>
                )}
              />
              
              {/* Action Buttons */}
              <div className="flex justify-between pt-6">
                <Button 
                  type="button"
                  variant="destructive"
                  onClick={onDeleteRequest}
                  className="flex items-center"
                >
                  <Trash2 className="mr-2 h-4 w-4" />
                  Удалить станцию
                </Button>
                <div className="flex space-x-3">
                  <Button 
                    type="button" 
                    variant="ghost"
                    onClick={onClose}
                  >
                    Отмена
                  </Button>
                  <Button 
                    type="submit"
                    disabled={updateStationMutation.isPending}
                    className="bg-blue-600 hover:bg-blue-700"
                  >
                    {updateStationMutation.isPending ? "Сохранение..." : "Сохранить изменения"}
                  </Button>
                </div>
              </div>
            </form>
          </Form>
        </div>
      </DialogContent>
    </Dialog>
  );
}
