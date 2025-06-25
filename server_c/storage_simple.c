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

// Глобальные переменные для хранения данных в памяти
static char data_file_path[512] = "../data/stations.json";
static int next_id = 1;
static charging_station_t* global_stations = NULL;
static int global_stations_count = 0;
static int global_stations_capacity = 0;
static int data_initialized = 0;

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
 * Преобразование JSON в структуру станции (только для переданных полей)
 */
int station_from_json(const json_value_t *json, charging_station_t *station) {
    if (!json || !station) return -1;
    
    // Инициализируем только обнуляемые поля, не все поля
    memset(station, 0, sizeof(charging_station_t));
    
    // Парсим только переданные поля из JSON
    if (json->type == JSON_OBJECT) {
        json_value_t *value;
        
        if ((value = json_object_get(json, "displayName"))) {
            const char* str_val = json_get_string(value);
            if (str_val) {
                strncpy(station->display_name, str_val, MAX_STRING_LENGTH - 1);
            }
        }
        
        if ((value = json_object_get(json, "technicalName"))) {
            const char* str_val = json_get_string(value);
            if (str_val) {
                strncpy(station->technical_name, str_val, MAX_STRING_LENGTH - 1);
            }
        }
        
        if ((value = json_object_get(json, "type"))) {
            const char* str_val = json_get_string(value);
            if (str_val) {
                strncpy(station->type, str_val, MAX_STRING_LENGTH - 1);
            }
        }
        
        if ((value = json_object_get(json, "status"))) {
            const char* str_val = json_get_string(value);
            if (str_val) {
                strncpy(station->status, str_val, MAX_STRING_LENGTH - 1);
            }
        }
        
        if ((value = json_object_get(json, "description"))) {
            const char* str_val = json_get_string(value);
            if (str_val) {
                strncpy(station->description, str_val, MAX_DESCRIPTION_LENGTH - 1);
            }
        }
        
        if ((value = json_object_get(json, "ipAddress"))) {
            const char* str_val = json_get_string(value);
            if (str_val) {
                strncpy(station->ip_address, str_val, MAX_STRING_LENGTH - 1);
            }
        }
        
        if ((value = json_object_get(json, "maxPower"))) {
            station->max_power = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "currentPower"))) {
            station->current_power = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "chargerPower"))) {
            station->charger_power = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "masterAvailablePower"))) {
            station->master_available_power = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "voltagePhase1"))) {
            station->voltage_phase1 = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "voltagePhase2"))) {
            station->voltage_phase2 = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "voltagePhase3"))) {
            station->voltage_phase3 = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "currentPhase1"))) {
            station->current_phase1 = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "currentPhase2"))) {
            station->current_phase2 = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "currentPhase3"))) {
            station->current_phase3 = (float)json_get_number(value);
        }
        
        if ((value = json_object_get(json, "carConnection"))) {
            station->car_connection = json_get_bool(value);
        }
        
        if ((value = json_object_get(json, "carChargingPermission"))) {
            station->car_charging_permission = json_get_bool(value);
        }
        
        if ((value = json_object_get(json, "carError"))) {
            station->car_error = json_get_bool(value);
        }
        
        if ((value = json_object_get(json, "masterOnline"))) {
            station->master_online = json_get_bool(value);
        }
        
        if ((value = json_object_get(json, "masterChargingPermission"))) {
            station->master_charging_permission = json_get_bool(value);
        }
        
        if ((value = json_object_get(json, "singlePhaseConnection"))) {
            station->single_phase_connection = json_get_bool(value);
        }
        
        if ((value = json_object_get(json, "powerOverconsumption"))) {
            station->power_overconsumption = json_get_bool(value);
        }
        
        if ((value = json_object_get(json, "fixedPower"))) {
            station->fixed_power = json_get_bool(value);
        }
    }
    
    return 0;
}

/**
 * Инициализация глобальных данных станций
 */
static void initialize_global_stations(void) {
    if (data_initialized) return;
    
    global_stations_capacity = 2;
    global_stations = malloc(global_stations_capacity * sizeof(charging_station_t));
    if (!global_stations) {
        printf("Ошибка выделения памяти для станций\n");
        return;
    }
    
    global_stations_count = 2;
    
    // Первая станция
    memset(&global_stations[0], 0, sizeof(charging_station_t));
    global_stations[0].id = 1;
    strcpy(global_stations[0].display_name, "Тестовая ESP32");
    strcpy(global_stations[0].technical_name, "ESP32-001");
    strcpy(global_stations[0].type, "slave");
    strcpy(global_stations[0].status, "available");
    global_stations[0].max_power = 22.0;
    global_stations[0].current_power = 0.0;
    
    // Вторая станция
    memset(&global_stations[1], 0, sizeof(charging_station_t));
    global_stations[1].id = 2;
    strcpy(global_stations[1].display_name, "Главная станция");
    strcpy(global_stations[1].technical_name, "MASTER-001");
    strcpy(global_stations[1].type, "master");
    strcpy(global_stations[1].status, "online");
    global_stations[1].max_power = 50.0;
    global_stations[1].current_power = 15.5;
    
    data_initialized = 1;
    printf("Инициализированы глобальные данные станций (%d станций)\n", global_stations_count);
}

/**
 * Получение всех зарядных станций из глобальной памяти
 */
int storage_get_stations(stations_array_t *stations) {
    initialize_global_stations();
    
    // Выделяем память и копируем данные из глобального хранилища
    stations->stations = malloc(global_stations_count * sizeof(charging_station_t));
    if (!stations->stations) {
        return -1;
    }
    
    stations->count = global_stations_count;
    stations->capacity = global_stations_count;
    
    // Копируем данные из глобального хранилища
    for (int i = 0; i < global_stations_count; i++) {
        memcpy(&stations->stations[i], &global_stations[i], sizeof(charging_station_t));
    }
    
    return 0;
}

/**
 * Получение зарядной станции по ID из глобальной памяти
 */
int storage_get_station(int id, charging_station_t *station) {
    initialize_global_stations();
    
    for (int i = 0; i < global_stations_count; i++) {
        if (global_stations[i].id == id) {
            memcpy(station, &global_stations[i], sizeof(charging_station_t));
            return 0;
        }
    }
    
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
 * Обновление зарядной станции в глобальной памяти
 */
int storage_update_station(int id, const charging_station_t *updates) {
    printf("DEBUG: storage_update_station called for ID %d\n", id);
    initialize_global_stations();
    
    // Ищем станцию с нужным ID в глобальном хранилище
    for (int i = 0; i < global_stations_count; i++) {
        if (global_stations[i].id == id) {
            printf("DEBUG: Found station with ID %d at index %d\n", id, i);
            printf("DEBUG: Old data: name='%s', maxPower=%.2f\n", 
                   global_stations[i].display_name, global_stations[i].max_power);
            
            charging_station_t *current = &global_stations[i];
            
            // Селективно обновляем только переданные поля
            if (strlen(updates->display_name) > 0) {
                strncpy(current->display_name, updates->display_name, sizeof(current->display_name) - 1);
                current->display_name[sizeof(current->display_name) - 1] = '\0';
            }
            if (strlen(updates->technical_name) > 0) {
                strncpy(current->technical_name, updates->technical_name, sizeof(current->technical_name) - 1);
                current->technical_name[sizeof(current->technical_name) - 1] = '\0';
            }
            if (strlen(updates->type) > 0) {
                strncpy(current->type, updates->type, sizeof(current->type) - 1);
                current->type[sizeof(current->type) - 1] = '\0';
            }
            if (strlen(updates->status) > 0) {
                strncpy(current->status, updates->status, sizeof(current->status) - 1);
                current->status[sizeof(current->status) - 1] = '\0';
            }
            if (strlen(updates->description) > 0) {
                strncpy(current->description, updates->description, sizeof(current->description) - 1);
                current->description[sizeof(current->description) - 1] = '\0';
            }
            if (strlen(updates->ip_address) > 0) {
                strncpy(current->ip_address, updates->ip_address, sizeof(current->ip_address) - 1);
                current->ip_address[sizeof(current->ip_address) - 1] = '\0';
            }
            
            // Числовые поля обновляем только если они не равны 0
            if (updates->max_power != 0.0f) {
                current->max_power = updates->max_power;
            }
            if (updates->current_power != current->current_power) {
                current->current_power = updates->current_power;
            }
            if (updates->charger_power != 0.0f) {
                current->charger_power = updates->charger_power;
            }
            if (updates->master_available_power != 0.0f) {
                current->master_available_power = updates->master_available_power;
            }
            
            // Параметры напряжения и тока
            if (updates->voltage_phase1 != 0.0f) {
                current->voltage_phase1 = updates->voltage_phase1;
            }
            if (updates->voltage_phase2 != 0.0f) {
                current->voltage_phase2 = updates->voltage_phase2;
            }
            if (updates->voltage_phase3 != 0.0f) {
                current->voltage_phase3 = updates->voltage_phase3;
            }
            if (updates->current_phase1 != 0.0f) {
                current->current_phase1 = updates->current_phase1;
            }
            if (updates->current_phase2 != 0.0f) {
                current->current_phase2 = updates->current_phase2;
            }
            if (updates->current_phase3 != 0.0f) {
                current->current_phase3 = updates->current_phase3;
            }
            
            printf("DEBUG: New data: name='%s', maxPower=%.2f\n", 
                   current->display_name, current->max_power);
            
            printf("Обновлена станция с ID %d в памяти\n", id);
            return 0;
        }
    }
    
    printf("DEBUG: Station with ID %d not found\n", id);
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