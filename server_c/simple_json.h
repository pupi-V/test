/**
 * Простая библиотека для работы с JSON
 * Заменяет cJSON для совместимости с Nix окружением
 */

#ifndef SIMPLE_JSON_H
#define SIMPLE_JSON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSON_MAX_STRING 1024
#define JSON_MAX_KEYS 50

/**
 * Типы JSON значений
 */
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type_t;

/**
 * Структура JSON значения
 */
typedef struct json_value {
    json_type_t type;
    union {
        int bool_val;
        double number_val;
        char *string_val;
        struct {
            struct json_value *items;
            int count;
        } array;
        struct {
            char **keys;
            struct json_value *values;
            int count;
        } object;
    } data;
} json_value_t;

// Создание JSON значений
json_value_t* json_create_null(void);
json_value_t* json_create_bool(int value);
json_value_t* json_create_number(double value);
json_value_t* json_create_string(const char *value);
json_value_t* json_create_array(void);
json_value_t* json_create_object(void);

// Работа с массивами
int json_array_add(json_value_t *array, json_value_t *item);
json_value_t* json_array_get(json_value_t *array, int index);
int json_array_size(json_value_t *array);

// Работа с объектами
int json_object_set(json_value_t *object, const char *key, json_value_t *value);
json_value_t* json_object_get(json_value_t *object, const char *key);
int json_object_has(json_value_t *object, const char *key);

// Получение значений
int json_get_bool(json_value_t *value);
double json_get_number(json_value_t *value);
const char* json_get_string(json_value_t *value);

// Парсинг и сериализация
json_value_t* json_parse(const char *json_string);
char* json_stringify(json_value_t *value);

// Освобождение памяти
void json_free(json_value_t *value);

// Утилиты
int json_is_null(json_value_t *value);
int json_is_bool(json_value_t *value);
int json_is_number(json_value_t *value);
int json_is_string(json_value_t *value);
int json_is_array(json_value_t *value);
int json_is_object(json_value_t *value);

#endif // SIMPLE_JSON_H