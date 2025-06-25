import { useMutation, useQueryClient } from "@tanstack/react-query";
import { type ChargingStation } from "../types/schema";
import { apiRequest } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import {
  Dialog,
  DialogContent,
} from "@/components/ui/dialog";
import { Button } from "@/components/ui/button";
import { AlertTriangle } from "lucide-react";

interface DeleteConfirmationModalProps {
  station: ChargingStation | null;
  onClose: () => void;
  onConfirm: () => void;
}

export default function DeleteConfirmationModal({ station, onClose, onConfirm }: DeleteConfirmationModalProps) {
  const { toast } = useToast();
  const queryClient = useQueryClient();

  const deleteStationMutation = useMutation({
    mutationFn: async () => {
      if (!station) throw new Error("No station selected");
      await apiRequest("DELETE", `/api/stations/${station.id}`);
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/stations"] });
      toast({
        title: "Станция удалена",
        description: "Зарядная станция была успешно удалена",
      });
      onConfirm();
    },
    onError: (error: any) => {
      toast({
        title: "Ошибка",
        description: error.message || "Не удалось удалить станцию",
        variant: "destructive",
      });
    },
  });

  const handleConfirm = () => {
    deleteStationMutation.mutate();
  };

  if (!station) return null;

  return (
    <Dialog open={!!station} onOpenChange={() => onClose()}>
      <DialogContent className="sm:max-w-md">
        <div className="p-6">
          <div className="flex items-center mb-4">
            <div className="w-12 h-12 bg-red-100 rounded-full flex items-center justify-center mr-4">
              <AlertTriangle className="text-red-600 text-xl" />
            </div>
            <h3 className="text-lg font-medium text-gray-900">Подтвердите удаление</h3>
          </div>
          <p className="text-gray-600 mb-6">
            Вы уверены, что хотите удалить станцию{" "}
            <strong>{station.displayName}</strong>?{" "}
            Это действие нельзя будет отменить.
          </p>
          <div className="flex justify-end space-x-3">
            <Button 
              type="button" 
              variant="ghost"
              onClick={onClose}
              disabled={deleteStationMutation.isPending}
            >
              Отмена
            </Button>
            <Button 
              variant="destructive"
              onClick={handleConfirm}
              disabled={deleteStationMutation.isPending}
            >
              {deleteStationMutation.isPending ? "Удаление..." : "Удалить"}
            </Button>
          </div>
        </div>
      </DialogContent>
    </Dialog>
  );
}
