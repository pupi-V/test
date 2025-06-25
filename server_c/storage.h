/**
 * Заголовочный файл для системы хранения данных зарядных станций
 * Поддерживает как JSON файлы, так и PostgreSQL базу данных
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "simple_json.h"

// Максимальные размеры строк
#define MAX_STRING_LENGTH 256
#define MAX_DESCRIPTION_LENGTH 1024
#define MAX_IP_LENGTH 16

/**
 * Структура данных зарядной станции
 */
typedef struct {
    int id;
    char display_name[MAX_STRING_LENGTH];
    char technical_name[MAX_STRING_LENGTH];
    char type[32]; // "master", "slave", "undefined"
    char status[32]; // "available", "charging", "offline", "maintenance"
    char ip_address[MAX_IP_LENGTH];
    char description[MAX_DESCRIPTION_LENGTH];
    
    float max_power;
    float current_power;
    
    // Данные для slave-станций
    int car_connection;
    int car_charging_permission;
    int car_error;
    int master_online;
    int master_charging_permission;
    float master_available_power;
    
    // Электрические параметры
    float voltage_phase1;
    float voltage_phase2;
    float voltage_phase3;
    float current_phase1;
    float current_phase2;
    float current_phase3;
    float charger_power;
    
    // Дополнительные параметры
    int single_phase_connection;
    int power_overconsumption;
    int fixed_power;
} charging_station_t;

/**
 * Структура для хранения массива станций
 */
typedef struct {
    charging_station_t *stations;
    int count;
    int capacity;
} stations_array_t;

// Функции инициализации и очистки
int storage_init(void);
void storage_cleanup(void);

// CRUD операции для зарядных станций
int storage_get_stations(stations_array_t *stations);
int storage_get_station(int id, charging_station_t *station);
int storage_create_station(const charging_station_t *station, int *new_id);
int storage_update_station(int id, const charging_station_t *updates);
int storage_delete_station(int id);

// Утилиты для работы с JSON
json_value_t* station_to_json(const charging_station_t *station);
int station_from_json(const json_value_t *json, charging_station_t *station);
void stations_array_free(stations_array_t *stations);

// Валидация данных
int validate_station_data(const charging_station_t *station);
int validate_update_data(const charging_station_t *updates);

// Функции для работы с глобальными данными
void initialize_global_stations(void);
int save_global_stations_to_file(void);

#endif // STORAGE_H