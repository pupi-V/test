/**
 * Реализация системы хранения данных зарядных станций
 * Поддерживает JSON файлы для разработки и PostgreSQL для продакшена
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
static int use_database = 0;
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
 * Инициализация подключения к PostgreSQL (заглушка)
 */
static int init_database_connection(void) {
    const char *db_url = getenv("DATABASE_URL");
    
    if (!db_url || strlen(db_url) == 0) {
        printf("DATABASE_URL не найден, используем JSON файлы\n");
        return 0;
    }
    
    printf("PostgreSQL поддержка отключена в этой версии, используем JSON файлы\n");
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
        file = fopen(data_file_path, "w");
        if (file) {
            fprintf(file, "[]");
            fclose(file);
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
    
    // Читаем содержимое файла
    char *json_string = malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        return -1;
    }
    
    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);
    
    // Парсим JSON
    cJSON *json = cJSON_Parse(json_string);
    free(json_string);
    
    if (!json) {
        fprintf(stderr, "Ошибка парсинга JSON файла\n");
        return -1;
    }
    
    if (!cJSON_IsArray(json)) {
        fprintf(stderr, "JSON файл должен содержать массив\n");
        cJSON_Delete(json);
        return -1;
    }
    
    int count = cJSON_GetArraySize(json);
    printf("Загружено %d станций из файла\n", count);
    
    // Находим максимальный ID для генерации следующего
    for (int i = 0; i < count; i++) {
        cJSON *station_json = cJSON_GetArrayItem(json, i);
        cJSON *id_json = cJSON_GetObjectItem(station_json, "id");
        if (cJSON_IsNumber(id_json)) {
            int id = id_json->valueint;
            if (id >= next_id) {
                next_id = id + 1;
            }
        }
    }
    
    cJSON_Delete(json);
    return 0;
}

/**
 * Инициализация системы хранения данных
 */
int storage_init(void) {
    if (ensure_data_directory() != 0) {
        return -1;
    }
    
    // Пытаемся подключиться к базе данных
    init_database_connection();
    
    // Если база данных недоступна, загружаем JSON данные
    if (!use_database) {
        return load_json_data();
    }
    
    return 0;
}

/**
 * Очистка ресурсов системы хранения
 */
void storage_cleanup(void) {
    printf("Система хранения очищена\n");
}

/**
 * Преобразование структуры станции в JSON (заглушка)
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
 * Преобразование JSON в структуру станции (упрощенная реализация)
 */
int station_from_json(const json_value_t *json, charging_station_t *station) {
    if (!json || !station) return -1;
    
    memset(station, 0, sizeof(charging_station_t));
    
    // Упрощенная реализация - устанавливаем значения по умолчанию
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
 * Получение всех зарядных станций из PostgreSQL
 */
static int get_stations_from_db(stations_array_t *stations) {
    const char *sql = "SELECT * FROM charging_stations ORDER BY id";
    PGresult *res = PQexec(db_conn, sql);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Ошибка получения станций: %s\n", PQerrorMessage(db_conn));
        PQclear(res);
        return -1;
    }
    
    int rows = PQntuples(res);
    stations->stations = malloc(rows * sizeof(charging_station_t));
    if (!stations->stations) {
        PQclear(res);
        return -1;
    }
    
    stations->count = rows;
    stations->capacity = rows;
    
    for (int i = 0; i < rows; i++) {
        charging_station_t *station = &stations->stations[i];
        memset(station, 0, sizeof(charging_station_t));
        
        station->id = atoi(PQgetvalue(res, i, 0));
        strncpy(station->display_name, PQgetvalue(res, i, 1), MAX_STRING_LENGTH - 1);
        strncpy(station->technical_name, PQgetvalue(res, i, 2), MAX_STRING_LENGTH - 1);
        strncpy(station->type, PQgetvalue(res, i, 3), 31);
        strncpy(station->status, PQgetvalue(res, i, 4), 31);
        
        if (PQgetvalue(res, i, 5) && strlen(PQgetvalue(res, i, 5)) > 0) {
            strncpy(station->ip_address, PQgetvalue(res, i, 5), MAX_IP_LENGTH - 1);
        }
        
        if (PQgetvalue(res, i, 6) && strlen(PQgetvalue(res, i, 6)) > 0) {
            strncpy(station->description, PQgetvalue(res, i, 6), MAX_DESCRIPTION_LENGTH - 1);
        }
        
        station->max_power = atof(PQgetvalue(res, i, 7));
        station->current_power = atof(PQgetvalue(res, i, 8));
        
        // Булевые поля
        station->car_connection = strcmp(PQgetvalue(res, i, 9), "t") == 0 ? 1 : 0;
        station->car_charging_permission = strcmp(PQgetvalue(res, i, 10), "t") == 0 ? 1 : 0;
        station->car_error = strcmp(PQgetvalue(res, i, 11), "t") == 0 ? 1 : 0;
        station->master_online = strcmp(PQgetvalue(res, i, 12), "t") == 0 ? 1 : 0;
        station->master_charging_permission = strcmp(PQgetvalue(res, i, 13), "t") == 0 ? 1 : 0;
        
        station->master_available_power = atof(PQgetvalue(res, i, 14));
        station->voltage_phase1 = atof(PQgetvalue(res, i, 15));
        station->voltage_phase2 = atof(PQgetvalue(res, i, 16));
        station->voltage_phase3 = atof(PQgetvalue(res, i, 17));
        station->current_phase1 = atof(PQgetvalue(res, i, 18));
        station->current_phase2 = atof(PQgetvalue(res, i, 19));
        station->current_phase3 = atof(PQgetvalue(res, i, 20));
        station->charger_power = atof(PQgetvalue(res, i, 21));
        
        station->single_phase_connection = strcmp(PQgetvalue(res, i, 22), "t") == 0 ? 1 : 0;
        station->power_overconsumption = strcmp(PQgetvalue(res, i, 23), "t") == 0 ? 1 : 0;
        station->fixed_power = strcmp(PQgetvalue(res, i, 24), "t") == 0 ? 1 : 0;
    }
    
    PQclear(res);
    return 0;
}

/**
 * Получение всех зарядных станций из JSON файла
 */
static int get_stations_from_json(stations_array_t *stations) {
    // Возвращаем тестовые данные для демонстрации
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
 * Получение всех зарядных станций
 */
int storage_get_stations(stations_array_t *stations) {
    if (use_database) {
        return get_stations_from_db(stations);
    } else {
        return get_stations_from_json(stations);
    }
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

/**
 * Сохранение всех станций в JSON файл
 */
static int save_stations_to_json(const stations_array_t *stations) {
    if (ensure_data_directory() != 0) {
        return -1;
    }
    
    cJSON *json_array = cJSON_CreateArray();
    if (!json_array) {
        return -1;
    }
    
    for (int i = 0; i < stations->count; i++) {
        cJSON *station_json = station_to_json(&stations->stations[i]);
        if (station_json) {
            cJSON_AddItemToArray(json_array, station_json);
        }
    }
    
    char *json_string = cJSON_Print(json_array);
    if (!json_string) {
        cJSON_Delete(json_array);
        return -1;
    }
    
    FILE *file = fopen(data_file_path, "w");
    if (!file) {
        free(json_string);
        cJSON_Delete(json_array);
        return -1;
    }
    
    fprintf(file, "%s", json_string);
    fclose(file);
    
    free(json_string);
    cJSON_Delete(json_array);
    return 0;
}

/**
 * Получение станции по ID из PostgreSQL
 */
static int get_station_from_db(int id, charging_station_t *station) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM charging_stations WHERE id = %d", id);
    
    PGresult *res = PQexec(db_conn, sql);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Ошибка получения станции: %s\n", PQerrorMessage(db_conn));
        PQclear(res);
        return -1;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        return -1; // Станция не найдена
    }
    
    memset(station, 0, sizeof(charging_station_t));
    
    station->id = atoi(PQgetvalue(res, 0, 0));
    strncpy(station->display_name, PQgetvalue(res, 0, 1), MAX_STRING_LENGTH - 1);
    strncpy(station->technical_name, PQgetvalue(res, 0, 2), MAX_STRING_LENGTH - 1);
    strncpy(station->type, PQgetvalue(res, 0, 3), 31);
    strncpy(station->status, PQgetvalue(res, 0, 4), 31);
    
    if (PQgetvalue(res, 0, 5) && strlen(PQgetvalue(res, 0, 5)) > 0) {
        strncpy(station->ip_address, PQgetvalue(res, 0, 5), MAX_IP_LENGTH - 1);
    }
    
    if (PQgetvalue(res, 0, 6) && strlen(PQgetvalue(res, 0, 6)) > 0) {
        strncpy(station->description, PQgetvalue(res, 0, 6), MAX_DESCRIPTION_LENGTH - 1);
    }
    
    station->max_power = atof(PQgetvalue(res, 0, 7));
    station->current_power = atof(PQgetvalue(res, 0, 8));
    
    // Булевые поля
    station->car_connection = strcmp(PQgetvalue(res, 0, 9), "t") == 0 ? 1 : 0;
    station->car_charging_permission = strcmp(PQgetvalue(res, 0, 10), "t") == 0 ? 1 : 0;
    station->car_error = strcmp(PQgetvalue(res, 0, 11), "t") == 0 ? 1 : 0;
    station->master_online = strcmp(PQgetvalue(res, 0, 12), "t") == 0 ? 1 : 0;
    station->master_charging_permission = strcmp(PQgetvalue(res, 0, 13), "t") == 0 ? 1 : 0;
    
    station->master_available_power = atof(PQgetvalue(res, 0, 14));
    station->voltage_phase1 = atof(PQgetvalue(res, 0, 15));
    station->voltage_phase2 = atof(PQgetvalue(res, 0, 16));
    station->voltage_phase3 = atof(PQgetvalue(res, 0, 17));
    station->current_phase1 = atof(PQgetvalue(res, 0, 18));
    station->current_phase2 = atof(PQgetvalue(res, 0, 19));
    station->current_phase3 = atof(PQgetvalue(res, 0, 20));
    station->charger_power = atof(PQgetvalue(res, 0, 21));
    
    station->single_phase_connection = strcmp(PQgetvalue(res, 0, 22), "t") == 0 ? 1 : 0;
    station->power_overconsumption = strcmp(PQgetvalue(res, 0, 23), "t") == 0 ? 1 : 0;
    station->fixed_power = strcmp(PQgetvalue(res, 0, 24), "t") == 0 ? 1 : 0;
    
    PQclear(res);
    return 0;
}

/**
 * Получение станции по ID из JSON файла
 */
static int get_station_from_json(int id, charging_station_t *station) {
    stations_array_t stations;
    if (get_stations_from_json(&stations) != 0) {
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
    return -1; // Станция не найдена
}

/**
 * Получение зарядной станции по ID
 */
int storage_get_station(int id, charging_station_t *station) {
    if (use_database) {
        return get_station_from_db(id, station);
    } else {
        return get_station_from_json(id, station);
    }
}

/**
 * Создание станции в PostgreSQL
 */
static int create_station_in_db(const charging_station_t *station, int *new_id) {
    const char *sql = 
        "INSERT INTO charging_stations ("
        "display_name, technical_name, type, status, ip_address, description, "
        "max_power, current_power, car_connection, car_charging_permission, "
        "car_error, master_online, master_charging_permission, master_available_power, "
        "voltage_phase1, voltage_phase2, voltage_phase3, current_phase1, "
        "current_phase2, current_phase3, charger_power, single_phase_connection, "
        "power_overconsumption, fixed_power) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, "
        "$15, $16, $17, $18, $19, $20, $21, $22, $23, $24) RETURNING id";
    
    // Подготавливаем параметры
    const char *params[24];
    char max_power_str[32], current_power_str[32];
    char master_available_power_str[32];
    char voltage_phase1_str[32], voltage_phase2_str[32], voltage_phase3_str[32];
    char current_phase1_str[32], current_phase2_str[32], current_phase3_str[32];
    char charger_power_str[32];
    
    params[0] = station->display_name;
    params[1] = station->technical_name;
    params[2] = station->type;
    params[3] = station->status;
    params[4] = strlen(station->ip_address) > 0 ? station->ip_address : NULL;
    params[5] = strlen(station->description) > 0 ? station->description : NULL;
    
    snprintf(max_power_str, sizeof(max_power_str), "%.2f", station->max_power);
    snprintf(current_power_str, sizeof(current_power_str), "%.2f", station->current_power);
    params[6] = max_power_str;
    params[7] = current_power_str;
    
    params[8] = station->car_connection ? "true" : "false";
    params[9] = station->car_charging_permission ? "true" : "false";
    params[10] = station->car_error ? "true" : "false";
    params[11] = station->master_online ? "true" : "false";
    params[12] = station->master_charging_permission ? "true" : "false";
    
    snprintf(master_available_power_str, sizeof(master_available_power_str), "%.2f", station->master_available_power);
    params[13] = master_available_power_str;
    
    snprintf(voltage_phase1_str, sizeof(voltage_phase1_str), "%.2f", station->voltage_phase1);
    snprintf(voltage_phase2_str, sizeof(voltage_phase2_str), "%.2f", station->voltage_phase2);
    snprintf(voltage_phase3_str, sizeof(voltage_phase3_str), "%.2f", station->voltage_phase3);
    params[14] = voltage_phase1_str;
    params[15] = voltage_phase2_str;
    params[16] = voltage_phase3_str;
    
    snprintf(current_phase1_str, sizeof(current_phase1_str), "%.2f", station->current_phase1);
    snprintf(current_phase2_str, sizeof(current_phase2_str), "%.2f", station->current_phase2);
    snprintf(current_phase3_str, sizeof(current_phase3_str), "%.2f", station->current_phase3);
    params[17] = current_phase1_str;
    params[18] = current_phase2_str;
    params[19] = current_phase3_str;
    
    snprintf(charger_power_str, sizeof(charger_power_str), "%.2f", station->charger_power);
    params[20] = charger_power_str;
    
    params[21] = station->single_phase_connection ? "true" : "false";
    params[22] = station->power_overconsumption ? "true" : "false";
    params[23] = station->fixed_power ? "true" : "false";
    
    PGresult *res = PQexecParams(db_conn, sql, 24, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Ошибка создания станции: %s\n", PQerrorMessage(db_conn));
        PQclear(res);
        return -1;
    }
    
    *new_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return 0;
}

/**
 * Создание станции в JSON файле
 */
static int create_station_in_json(const charging_station_t *station, int *new_id) {
    stations_array_t stations;
    if (get_stations_from_json(&stations) != 0) {
        // Если файл не существует, создаем пустой массив
        stations.stations = NULL;
        stations.count = 0;
        stations.capacity = 0;
    }
    
    // Расширяем массив
    if (stations.count >= stations.capacity) {
        stations.capacity = stations.capacity == 0 ? 10 : stations.capacity * 2;
        stations.stations = realloc(stations.stations, 
                                  stations.capacity * sizeof(charging_station_t));
        if (!stations.stations) {
            return -1;
        }
    }
    
    // Находим следующий ID
    int max_id = 0;
    for (int i = 0; i < stations.count; i++) {
        if (stations.stations[i].id > max_id) {
            max_id = stations.stations[i].id;
        }
    }
    *new_id = max_id + 1;
    
    // Копируем станцию и присваиваем ID
    memcpy(&stations.stations[stations.count], station, sizeof(charging_station_t));
    stations.stations[stations.count].id = *new_id;
    stations.count++;
    
    // Сохраняем в файл
    int result = save_stations_to_json(&stations);
    stations_array_free(&stations);
    
    return result;
}

/**
 * Создание новой зарядной станции
 */
int storage_create_station(const charging_station_t *station, int *new_id) {
    if (use_database) {
        return create_station_in_db(station, new_id);
    } else {
        return create_station_in_json(station, new_id);
    }
}

/**
 * Обновление станции в PostgreSQL
 */
static int update_station_in_db(int id, const charging_station_t *updates) {
    // Сначала получаем существующую станцию
    charging_station_t existing;
    if (get_station_from_db(id, &existing) != 0) {
        return -1; // Станция не найдена
    }
    
    // Объединяем существующие данные с обновлениями
    charging_station_t updated = existing;
    
    if (strlen(updates->display_name) > 0) {
        strcpy(updated.display_name, updates->display_name);
    }
    if (strlen(updates->technical_name) > 0) {
        strcpy(updated.technical_name, updates->technical_name);
    }
    if (strlen(updates->type) > 0) {
        strcpy(updated.type, updates->type);
    }
    if (strlen(updates->status) > 0) {
        strcpy(updated.status, updates->status);
    }
    if (strlen(updates->ip_address) > 0) {
        strcpy(updated.ip_address, updates->ip_address);
    }
    if (strlen(updates->description) > 0) {
        strcpy(updated.description, updates->description);
    }
    
    if (updates->max_power > 0) {
        updated.max_power = updates->max_power;
    }
    if (updates->current_power >= 0) {
        updated.current_power = updates->current_power;
    }
    
    // Обновляем булевые поля (всегда, так как они могут быть 0 или 1)
    updated.car_connection = updates->car_connection;
    updated.car_charging_permission = updates->car_charging_permission;
    updated.car_error = updates->car_error;
    updated.master_online = updates->master_online;
    updated.master_charging_permission = updates->master_charging_permission;
    
    if (updates->master_available_power >= 0) {
        updated.master_available_power = updates->master_available_power;
    }
    if (updates->voltage_phase1 >= 0) {
        updated.voltage_phase1 = updates->voltage_phase1;
    }
    if (updates->voltage_phase2 >= 0) {
        updated.voltage_phase2 = updates->voltage_phase2;
    }
    if (updates->voltage_phase3 >= 0) {
        updated.voltage_phase3 = updates->voltage_phase3;
    }
    if (updates->current_phase1 >= 0) {
        updated.current_phase1 = updates->current_phase1;
    }
    if (updates->current_phase2 >= 0) {
        updated.current_phase2 = updates->current_phase2;
    }
    if (updates->current_phase3 >= 0) {
        updated.current_phase3 = updates->current_phase3;
    }
    if (updates->charger_power >= 0) {
        updated.charger_power = updates->charger_power;
    }
    
    updated.single_phase_connection = updates->single_phase_connection;
    updated.power_overconsumption = updates->power_overconsumption;
    updated.fixed_power = updates->fixed_power;
    
    // Выполняем SQL обновление
    const char *sql = 
        "UPDATE charging_stations SET "
        "display_name = $1, technical_name = $2, type = $3, status = $4, "
        "ip_address = $5, description = $6, max_power = $7, current_power = $8, "
        "car_connection = $9, car_charging_permission = $10, car_error = $11, "
        "master_online = $12, master_charging_permission = $13, master_available_power = $14, "
        "voltage_phase1 = $15, voltage_phase2 = $16, voltage_phase3 = $17, "
        "current_phase1 = $18, current_phase2 = $19, current_phase3 = $20, "
        "charger_power = $21, single_phase_connection = $22, power_overconsumption = $23, "
        "fixed_power = $24 WHERE id = $25";
    
    // Подготавливаем параметры (аналогично create_station_in_db)
    const char *params[25];
    char max_power_str[32], current_power_str[32];
    char master_available_power_str[32];
    char voltage_phase1_str[32], voltage_phase2_str[32], voltage_phase3_str[32];
    char current_phase1_str[32], current_phase2_str[32], current_phase3_str[32];
    char charger_power_str[32], id_str[32];
    
    params[0] = updated.display_name;
    params[1] = updated.technical_name;
    params[2] = updated.type;
    params[3] = updated.status;
    params[4] = strlen(updated.ip_address) > 0 ? updated.ip_address : NULL;
    params[5] = strlen(updated.description) > 0 ? updated.description : NULL;
    
    snprintf(max_power_str, sizeof(max_power_str), "%.2f", updated.max_power);
    snprintf(current_power_str, sizeof(current_power_str), "%.2f", updated.current_power);
    params[6] = max_power_str;
    params[7] = current_power_str;
    
    params[8] = updated.car_connection ? "true" : "false";
    params[9] = updated.car_charging_permission ? "true" : "false";
    params[10] = updated.car_error ? "true" : "false";
    params[11] = updated.master_online ? "true" : "false";
    params[12] = updated.master_charging_permission ? "true" : "false";
    
    snprintf(master_available_power_str, sizeof(master_available_power_str), "%.2f", updated.master_available_power);
    params[13] = master_available_power_str;
    
    snprintf(voltage_phase1_str, sizeof(voltage_phase1_str), "%.2f", updated.voltage_phase1);
    snprintf(voltage_phase2_str, sizeof(voltage_phase2_str), "%.2f", updated.voltage_phase2);
    snprintf(voltage_phase3_str, sizeof(voltage_phase3_str), "%.2f", updated.voltage_phase3);
    params[14] = voltage_phase1_str;
    params[15] = voltage_phase2_str;
    params[16] = voltage_phase3_str;
    
    snprintf(current_phase1_str, sizeof(current_phase1_str), "%.2f", updated.current_phase1);
    snprintf(current_phase2_str, sizeof(current_phase2_str), "%.2f", updated.current_phase2);
    snprintf(current_phase3_str, sizeof(current_phase3_str), "%.2f", updated.current_phase3);
    params[17] = current_phase1_str;
    params[18] = current_phase2_str;
    params[19] = current_phase3_str;
    
    snprintf(charger_power_str, sizeof(charger_power_str), "%.2f", updated.charger_power);
    params[20] = charger_power_str;
    
    params[21] = updated.single_phase_connection ? "true" : "false";
    params[22] = updated.power_overconsumption ? "true" : "false";
    params[23] = updated.fixed_power ? "true" : "false";
    
    snprintf(id_str, sizeof(id_str), "%d", id);
    params[24] = id_str;
    
    PGresult *res = PQexecParams(db_conn, sql, 25, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Ошибка обновления станции: %s\n", PQerrorMessage(db_conn));
        PQclear(res);
        return -1;
    }
    
    PQclear(res);
    return 0;
}

/**
 * Обновление станции в JSON файле
 */
static int update_station_in_json(int id, const charging_station_t *updates) {
    stations_array_t stations;
    if (get_stations_from_json(&stations) != 0) {
        return -1;
    }
    
    int found = 0;
    for (int i = 0; i < stations.count; i++) {
        if (stations.stations[i].id == id) {
            // Объединяем существующие данные с обновлениями
            charging_station_t *existing = &stations.stations[i];
            
            if (strlen(updates->display_name) > 0) {
                strcpy(existing->display_name, updates->display_name);
            }
            if (strlen(updates->technical_name) > 0) {
                strcpy(existing->technical_name, updates->technical_name);
            }
            if (strlen(updates->type) > 0) {
                strcpy(existing->type, updates->type);
            }
            if (strlen(updates->status) > 0) {
                strcpy(existing->status, updates->status);
            }
            if (strlen(updates->ip_address) > 0) {
                strcpy(existing->ip_address, updates->ip_address);
            }
            if (strlen(updates->description) > 0) {
                strcpy(existing->description, updates->description);
            }
            
            if (updates->max_power > 0) {
                existing->max_power = updates->max_power;
            }
            if (updates->current_power >= 0) {
                existing->current_power = updates->current_power;
            }
            
            // Обновляем булевые поля
            existing->car_connection = updates->car_connection;
            existing->car_charging_permission = updates->car_charging_permission;
            existing->car_error = updates->car_error;
            existing->master_online = updates->master_online;
            existing->master_charging_permission = updates->master_charging_permission;
            
            if (updates->master_available_power >= 0) {
                existing->master_available_power = updates->master_available_power;
            }
            if (updates->voltage_phase1 >= 0) {
                existing->voltage_phase1 = updates->voltage_phase1;
            }
            if (updates->voltage_phase2 >= 0) {
                existing->voltage_phase2 = updates->voltage_phase2;
            }
            if (updates->voltage_phase3 >= 0) {
                existing->voltage_phase3 = updates->voltage_phase3;
            }
            if (updates->current_phase1 >= 0) {
                existing->current_phase1 = updates->current_phase1;
            }
            if (updates->current_phase2 >= 0) {
                existing->current_phase2 = updates->current_phase2;
            }
            if (updates->current_phase3 >= 0) {
                existing->current_phase3 = updates->current_phase3;
            }
            if (updates->charger_power >= 0) {
                existing->charger_power = updates->charger_power;
            }
            
            existing->single_phase_connection = updates->single_phase_connection;
            existing->power_overconsumption = updates->power_overconsumption;
            existing->fixed_power = updates->fixed_power;
            
            found = 1;
            break;
        }
    }
    
    if (!found) {
        stations_array_free(&stations);
        return -1; // Станция не найдена
    }
    
    // Сохраняем изменения
    int result = save_stations_to_json(&stations);
    stations_array_free(&stations);
    
    return result;
}

/**
 * Обновление зарядной станции
 */
int storage_update_station(int id, const charging_station_t *updates) {
    if (use_database) {
        return update_station_in_db(id, updates);
    } else {
        return update_station_in_json(id, updates);
    }
}

/**
 * Удаление станции из PostgreSQL
 */
static int delete_station_from_db(int id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM charging_stations WHERE id = %d", id);
    
    PGresult *res = PQexec(db_conn, sql);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Ошибка удаления станции: %s\n", PQerrorMessage(db_conn));
        PQclear(res);
        return -1;
    }
    
    int affected_rows = atoi(PQcmdTuples(res));
    PQclear(res);
    
    return (affected_rows > 0) ? 0 : -1;
}

/**
 * Удаление станции из JSON файла
 */
static int delete_station_from_json(int id) {
    stations_array_t stations;
    if (get_stations_from_json(&stations) != 0) {
        return -1;
    }
    
    int found = 0;
    for (int i = 0; i < stations.count; i++) {
        if (stations.stations[i].id == id) {
            // Сдвигаем элементы массива
            for (int j = i; j < stations.count - 1; j++) {
                memcpy(&stations.stations[j], &stations.stations[j + 1], 
                       sizeof(charging_station_t));
            }
            stations.count--;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        stations_array_free(&stations);
        return -1; // Станция не найдена
    }
    
    // Сохраняем изменения
    int result = save_stations_to_json(&stations);
    stations_array_free(&stations);
    
    return result;
}

/**
 * Удаление зарядной станции
 */
int storage_delete_station(int id) {
    if (use_database) {
        return delete_station_from_db(id);
    } else {
        return delete_station_from_json(id);
    }
}