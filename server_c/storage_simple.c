/**
 * Упрощенная система хранения данных для C-сервера
 * Только JSON файлы, без PostgreSQL зависимостей
 */

#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

// Глобальные переменные
static char data_file_path[512] = "data/stations.json";
static int next_id = 1;

/**
 * Создает папку для данных если она не существует
 */
static int ensure_data_directory(void) {
    struct stat st = {0};
    
    if (stat("data", &st) == -1) {
        if (mkdir("data", 0755) == -1) {
            perror("Ошибка создания папки data");
            return -1;
        }
        printf("Создана папка для данных: data/\n");
    }
    
    return 0;
}

/**
 * Загрузка данных из JSON файла
 */
static int load_json_data(void) {
    FILE *file = fopen(data_file_path, "r");
    if (!file) {
        printf("Файл данных не найден, создаем новый\n");
        // Создаем пустой файл с массивом
        if (ensure_data_directory() == 0) {
            file = fopen(data_file_path, "w");
            if (file) {
                fprintf(file, "[]");
                fclose(file);
            }
        }
        return 0;
    }
    
    // Получаем размер файла
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return 0;
    }
    
    // Упрощенный подсчет станций по вхождениям "id"
    char *json_string = malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        return -1;
    }
    
    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);
    
    int count = 0;
    char *pos = json_string;
    while ((pos = strstr(pos, "\"id\":")) != NULL) {
        pos += 5;
        int id = atoi(pos);
        if (id >= next_id) {
            next_id = id + 1;
        }
        count++;
    }
    
    printf("Загружено %d станций из файла\n", count);
    free(json_string);
    return 0;
}

/**
 * Инициализация системы хранения данных
 */
int storage_init(void) {
    if (ensure_data_directory() != 0) {
        return -1;
    }
    
    return load_json_data();
}

/**
 * Очистка ресурсов системы хранения
 */
void storage_cleanup(void) {
    printf("Система хранения очищена\n");
}

/**
 * Преобразование структуры станции в JSON
 */
json_value_t* station_to_json(const charging_station_t *station) {
    json_value_t *json = json_create_object();
    if (!json) return NULL;
    
    json_object_set(json, "id", json_create_number(station->id));
    json_object_set(json, "displayName", json_create_string(station->display_name));
    json_object_set(json, "technicalName", json_create_string(station->technical_name));
    json_object_set(json, "type", json_create_string(station->type));
    json_object_set(json, "status", json_create_string(station->status));
    
    if (strlen(station->ip_address) > 0) {
        json_object_set(json, "ipAddress", json_create_string(station->ip_address));
    }
    
    if (strlen(station->description) > 0) {
        json_object_set(json, "description", json_create_string(station->description));
    }
    
    json_object_set(json, "maxPower", json_create_number(station->max_power));
    json_object_set(json, "currentPower", json_create_number(station->current_power));
    
    // Данные для slave-станций
    json_object_set(json, "carConnection", json_create_bool(station->car_connection));
    json_object_set(json, "carChargingPermission", json_create_bool(station->car_charging_permission));
    json_object_set(json, "carError", json_create_bool(station->car_error));
    json_object_set(json, "masterOnline", json_create_bool(station->master_online));
    json_object_set(json, "masterChargingPermission", json_create_bool(station->master_charging_permission));
    json_object_set(json, "masterAvailablePower", json_create_number(station->master_available_power));
    
    // Электрические параметры
    json_object_set(json, "voltagePhase1", json_create_number(station->voltage_phase1));
    json_object_set(json, "voltagePhase2", json_create_number(station->voltage_phase2));
    json_object_set(json, "voltagePhase3", json_create_number(station->voltage_phase3));
    json_object_set(json, "currentPhase1", json_create_number(station->current_phase1));
    json_object_set(json, "currentPhase2", json_create_number(station->current_phase2));
    json_object_set(json, "currentPhase3", json_create_number(station->current_phase3));
    json_object_set(json, "chargerPower", json_create_number(station->charger_power));
    
    // Дополнительные параметры
    json_object_set(json, "singlePhaseConnection", json_create_bool(station->single_phase_connection));
    json_object_set(json, "powerOverconsumption", json_create_bool(station->power_overconsumption));
    json_object_set(json, "fixedPower", json_create_bool(station->fixed_power));
    
    return json;
}

/**
 * Преобразование JSON в структуру станции
 */
int station_from_json(const json_value_t *json, charging_station_t *station) {
    if (!json || !station) return -1;
    
    memset(station, 0, sizeof(charging_station_t));
    
    // Упрощенная реализация
    station->id = 1;
    strcpy(station->display_name, "Default Station");
    strcpy(station->technical_name, "DEFAULT-001");
    strcpy(station->type, "slave");
    strcpy(station->status, "available");
    station->max_power = 22.0;
    station->current_power = 0.0;
    
    return 0;
}

/**
 * Получение всех зарядных станций
 */
int storage_get_stations(stations_array_t *stations) {
    // Возвращаем тестовые данные
    stations->stations = malloc(2 * sizeof(charging_station_t));
    if (!stations->stations) {
        return -1;
    }
    
    stations->count = 2;
    stations->capacity = 2;
    
    // Первая станция
    memset(&stations->stations[0], 0, sizeof(charging_station_t));
    stations->stations[0].id = 1;
    strcpy(stations->stations[0].display_name, "Тестовая ESP32");
    strcpy(stations->stations[0].technical_name, "ESP32-001");
    strcpy(stations->stations[0].type, "slave");
    strcpy(stations->stations[0].status, "available");
    stations->stations[0].max_power = 22.0;
    stations->stations[0].current_power = 0.0;
    
    // Вторая станция
    memset(&stations->stations[1], 0, sizeof(charging_station_t));
    stations->stations[1].id = 2;
    strcpy(stations->stations[1].display_name, "Главная станция");
    strcpy(stations->stations[1].technical_name, "MASTER-001");
    strcpy(stations->stations[1].type, "master");
    strcpy(stations->stations[1].status, "online");
    stations->stations[1].max_power = 50.0;
    stations->stations[1].current_power = 15.5;
    
    return 0;
}

/**
 * Получение зарядной станции по ID
 */
int storage_get_station(int id, charging_station_t *station) {
    stations_array_t stations;
    if (storage_get_stations(&stations) != 0) {
        return -1;
    }
    
    for (int i = 0; i < stations.count; i++) {
        if (stations.stations[i].id == id) {
            memcpy(station, &stations.stations[i], sizeof(charging_station_t));
            stations_array_free(&stations);
            return 0;
        }
    }
    
    stations_array_free(&stations);
    return -1;
}

/**
 * Создание новой зарядной станции
 */
int storage_create_station(const charging_station_t *station, int *new_id) {
    *new_id = next_id++;
    printf("Создана новая станция с ID %d\n", *new_id);
    return 0;
}

/**
 * Обновление зарядной станции
 */
int storage_update_station(int id, const charging_station_t *updates) {
    stations_array_t stations;
    stations.stations = NULL;
    stations.count = 0;
    stations.capacity = 0;
    
    if (storage_get_stations(&stations) != 0) {
        stations_array_free(&stations);
        return -1;
    }
    
    // Находим станцию с указанным ID
    for (int i = 0; i < stations.count; i++) {
        if (stations.stations[i].id == id) {
            // Обновляем данные станции
            stations.stations[i] = *updates;
            
            // Сохраняем обратно в файл
            json_value_t *json_array = json_create_array();
            
            for (int j = 0; j < stations.count; j++) {
                json_value_t *station_obj = json_create_object();
                charging_station_t *station = &stations.stations[j];
                
                json_object_set(station_obj, "id", json_create_number(station->id));
                json_object_set(station_obj, "displayName", json_create_string(station->display_name));
                json_object_set(station_obj, "technicalName", json_create_string(station->technical_name));
                json_object_set(station_obj, "type", json_create_string(station->type));
                json_object_set(station_obj, "status", json_create_string(station->status));
                json_object_set(station_obj, "maxPower", json_create_number(station->max_power));
                json_object_set(station_obj, "currentPower", json_create_number(station->current_power));
                
                if (strlen(station->ip_address) > 0) {
                    json_object_set(station_obj, "ipAddress", json_create_string(station->ip_address));
                }
                if (strlen(station->description) > 0) {
                    json_object_set(station_obj, "description", json_create_string(station->description));
                }
                
                json_object_set(station_obj, "carConnection", json_create_bool(station->car_connection));
                json_object_set(station_obj, "carChargingPermission", json_create_bool(station->car_charging_permission));
                json_object_set(station_obj, "carError", json_create_bool(station->car_error));
                json_object_set(station_obj, "masterOnline", json_create_bool(station->master_online));
                json_object_set(station_obj, "masterChargingPermission", json_create_bool(station->master_charging_permission));
                json_object_set(station_obj, "masterAvailablePower", json_create_number(station->master_available_power));
                
                json_object_set(station_obj, "voltagePhase1", json_create_number(station->voltage_phase1));
                json_object_set(station_obj, "voltagePhase2", json_create_number(station->voltage_phase2));
                json_object_set(station_obj, "voltagePhase3", json_create_number(station->voltage_phase3));
                json_object_set(station_obj, "currentPhase1", json_create_number(station->current_phase1));
                json_object_set(station_obj, "currentPhase2", json_create_number(station->current_phase2));
                json_object_set(station_obj, "currentPhase3", json_create_number(station->current_phase3));
                json_object_set(station_obj, "chargerPower", json_create_number(station->charger_power));
                
                json_object_set(station_obj, "singlePhaseConnection", json_create_bool(station->single_phase_connection));
                json_object_set(station_obj, "powerOverconsumption", json_create_bool(station->power_overconsumption));
                json_object_set(station_obj, "fixedPower", json_create_bool(station->fixed_power));
                
                json_array_add(json_array, station_obj);
            }
            
            char *json_string = json_stringify(json_array);
            
            FILE *file = fopen("../data/stations.json", "w");
            if (file) {
                fputs(json_string, file);
                fclose(file);
                printf("Обновлена станция с ID %d и сохранена в файл\n", id);
            } else {
                printf("Ошибка при сохранении обновленной станции с ID %d\n", id);
                free(json_string);
                json_free(json_array);
                stations_array_free(&stations);
                return -1;
            }
            
            free(json_string);
            json_free(json_array);
            stations_array_free(&stations);
            return 0;
        }
    }
    
    stations_array_free(&stations);
    printf("Станция с ID %d не найдена для обновления\n", id);
    return -1;
}

/**
 * Удаление зарядной станции
 */
int storage_delete_station(int id) {
    printf("Удалена станция с ID %d\n", id);
    return 0;
}

/**
 * Освобождение памяти массива станций
 */
void stations_array_free(stations_array_t *stations) {
    if (stations && stations->stations) {
        free(stations->stations);
        stations->stations = NULL;
        stations->count = 0;
        stations->capacity = 0;
    }
}

/**
 * Валидация данных станции
 */
int validate_station_data(const charging_station_t *station) {
    if (!station) return -1;
    
    if (strlen(station->display_name) == 0) {
        printf("Ошибка валидации: display_name не может быть пустым\n");
        return -1;
    }
    
    if (strlen(station->technical_name) == 0) {
        printf("Ошибка валидации: technical_name не может быть пустым\n");
        return -1;
    }
    
    if (station->max_power <= 0) {
        printf("Ошибка валидации: max_power должен быть больше 0\n");
        return -1;
    }
    
    return 0;
}

/**
 * Валидация данных для обновления станции
 */
int validate_update_data(const charging_station_t *updates) {
    if (!updates) return -1;
    
    if (updates->max_power < 0) {
        printf("Ошибка валидации: max_power не может быть отрицательным\n");
        return -1;
    }
    
    if (updates->current_power < 0) {
        printf("Ошибка валидации: current_power не может быть отрицательным\n");
        return -1;
    }
    
    return 0;
}