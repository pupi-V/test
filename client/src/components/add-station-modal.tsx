import { useState } from "react";
import { useMutation, useQueryClient } from "@tanstack/react-query";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import { insertChargingStationSchema, type InsertChargingStation } from "../types/schema";
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

interface AddStationModalProps {
  open: boolean;
  onOpenChange: (open: boolean) => void;
}

export default function AddStationModal({ open, onOpenChange }: AddStationModalProps) {
  const { toast } = useToast();
  const queryClient = useQueryClient();

  const form = useForm<InsertChargingStation>({
    resolver: zodResolver(insertChargingStationSchema),
    defaultValues: {
      displayName: "",
      technicalName: "",
      type: "undefined",
      maxPower: 22,
      status: "available",
      ipAddress: "",
      description: "",
    },
  });

  const createStationMutation = useMutation({
    mutationFn: async (data: InsertChargingStation) => {
      const response = await apiRequest("POST", "/api/stations", data);
      return response.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/stations"] });
      toast({
        title: "Станция добавлена",
        description: "Новая зарядная станция успешно создана",
      });
      form.reset();
      onOpenChange(false);
    },
    onError: (error: any) => {
      toast({
        title: "Ошибка",
        description: error.message || "Не удалось создать станцию",
        variant: "destructive",
      });
    },
  });

  const onSubmit = (data: InsertChargingStation) => {
    createStationMutation.mutate(data);
  };

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="sm:max-w-md">
        <DialogHeader>
          <DialogTitle>Добавить новую станцию</DialogTitle>
        </DialogHeader>
        
        <Form {...form}>
          <form onSubmit={form.handleSubmit(onSubmit)} className="space-y-4">
            <FormField
              control={form.control}
              name="displayName"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>Отображаемое имя</FormLabel>
                  <FormControl>
                    <Input placeholder="Например: Станция A1" {...field} />
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
                    <Input placeholder="Например: CS-MASTER-001" {...field} />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            
            <div>
              <label className="text-sm font-medium leading-none peer-disabled:cursor-not-allowed peer-disabled:opacity-70">
                Тип станции
              </label>
              <Input 
                value="Неопределенный"
                disabled
                className="bg-gray-100 cursor-not-allowed mt-2"
              />
              <p className="text-sm text-gray-500 mt-1">Тип станции будет определен автоматически системой</p>
            </div>
            
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
                      placeholder="22.0"
                      {...field}
                      onChange={(e) => field.onChange(parseFloat(e.target.value) || 0)}
                    />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />

            <FormField
              control={form.control}
              name="ipAddress"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>IP адрес (опционально)</FormLabel>
                  <FormControl>
                    <Input placeholder="192.168.1.101" {...field} value={field.value || ""} />
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
                  <FormLabel>Описание (опционально)</FormLabel>
                  <FormControl>
                    <Textarea 
                      placeholder="Дополнительная информация о станции..."
                      rows={3}
                      {...field}
                      value={field.value || ""}
                    />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            
            <div className="flex justify-end space-x-3 pt-4">
              <Button 
                type="button" 
                variant="ghost"
                onClick={() => onOpenChange(false)}
              >
                Отмена
              </Button>
              <Button 
                type="submit"
                disabled={createStationMutation.isPending}
                className="bg-blue-600 hover:bg-blue-700"
              >
                {createStationMutation.isPending ? "Создание..." : "Добавить станцию"}
              </Button>
            </div>
          </form>
        </Form>
      </DialogContent>
    </Dialog>
  );
}
