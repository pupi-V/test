/**
 * Простая реализация JSON библиотеки
 * Базовая функциональность для работы с JSON в C-сервере
 */

#include "simple_json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Создание NULL значения
 */
json_value_t* json_create_null(void) {
    json_value_t *value = malloc(sizeof(json_value_t));
    if (value) {
        value->type = JSON_NULL;
    }
    return value;
}

/**
 * Создание булевого значения
 */
json_value_t* json_create_bool(int bool_value) {
    json_value_t *value = malloc(sizeof(json_value_t));
    if (value) {
        value->type = JSON_BOOL;
        value->data.bool_val = bool_value ? 1 : 0;
    }
    return value;
}

/**
 * Создание числового значения
 */
json_value_t* json_create_number(double num_value) {
    json_value_t *value = malloc(sizeof(json_value_t));
    if (value) {
        value->type = JSON_NUMBER;
        value->data.number_val = num_value;
    }
    return value;
}

/**
 * Создание строкового значения
 */
json_value_t* json_create_string(const char *str_value) {
    json_value_t *value = malloc(sizeof(json_value_t));
    if (value && str_value) {
        value->type = JSON_STRING;
        value->data.string_val = malloc(strlen(str_value) + 1);
        if (value->data.string_val) {
            strcpy(value->data.string_val, str_value);
        } else {
            free(value);
            return NULL;
        }
    }
    return value;
}

/**
 * Создание массива
 */
json_value_t* json_create_array(void) {
    json_value_t *value = malloc(sizeof(json_value_t));
    if (value) {
        value->type = JSON_ARRAY;
        value->data.array.items = NULL;
        value->data.array.count = 0;
    }
    return value;
}

/**
 * Создание объекта
 */
json_value_t* json_create_object(void) {
    json_value_t *value = malloc(sizeof(json_value_t));
    if (value) {
        value->type = JSON_OBJECT;
        value->data.object.keys = NULL;
        value->data.object.values = NULL;
        value->data.object.count = 0;
    }
    return value;
}

/**
 * Добавление элемента в массив
 */
int json_array_add(json_value_t *array, json_value_t *item) {
    if (!array || array->type != JSON_ARRAY || !item) {
        return -1;
    }
    
    int new_count = array->data.array.count + 1;
    json_value_t *new_items = realloc(array->data.array.items, 
                                     new_count * sizeof(json_value_t));
    if (!new_items) {
        return -1;
    }
    
    array->data.array.items = new_items;
    array->data.array.items[array->data.array.count] = *item;
    array->data.array.count = new_count;
    
    free(item); // Копируем содержимое, освобождаем обертку
    return 0;
}

/**
 * Получение элемента массива по индексу
 */
json_value_t* json_array_get(json_value_t *array, int index) {
    if (!array || array->type != JSON_ARRAY || 
        index < 0 || index >= array->data.array.count) {
        return NULL;
    }
    
    return &array->data.array.items[index];
}

/**
 * Размер массива
 */
int json_array_size(json_value_t *array) {
    if (!array || array->type != JSON_ARRAY) {
        return -1;
    }
    return array->data.array.count;
}

/**
 * Установка значения в объекте
 */
int json_object_set(json_value_t *object, const char *key, json_value_t *value) {
    if (!object || object->type != JSON_OBJECT || !key || !value) {
        return -1;
    }
    
    // Проверяем, существует ли уже такой ключ
    for (int i = 0; i < object->data.object.count; i++) {
        if (strcmp(object->data.object.keys[i], key) == 0) {
            // Заменяем существующее значение
            json_free(&object->data.object.values[i]);
            object->data.object.values[i] = *value;
            free(value);
            return 0;
        }
    }
    
    // Добавляем новый ключ-значение
    int new_count = object->data.object.count + 1;
    
    char **new_keys = realloc(object->data.object.keys, new_count * sizeof(char*));
    json_value_t *new_values = realloc(object->data.object.values, 
                                      new_count * sizeof(json_value_t));
    
    if (!new_keys || !new_values) {
        return -1;
    }
    
    new_keys[object->data.object.count] = malloc(strlen(key) + 1);
    if (!new_keys[object->data.object.count]) {
        return -1;
    }
    strcpy(new_keys[object->data.object.count], key);
    
    new_values[object->data.object.count] = *value;
    
    object->data.object.keys = new_keys;
    object->data.object.values = new_values;
    object->data.object.count = new_count;
    
    free(value);
    return 0;
}

/**
 * Получение значения из объекта по ключу
 */
json_value_t* json_object_get(json_value_t *object, const char *key) {
    if (!object || object->type != JSON_OBJECT || !key) {
        return NULL;
    }
    
    for (int i = 0; i < object->data.object.count; i++) {
        if (strcmp(object->data.object.keys[i], key) == 0) {
            return &object->data.object.values[i];
        }
    }
    
    return NULL;
}

/**
 * Проверка наличия ключа в объекте
 */
int json_object_has(json_value_t *object, const char *key) {
    return json_object_get(object, key) != NULL;
}

/**
 * Получение булевого значения
 */
int json_get_bool(json_value_t *value) {
    if (value && value->type == JSON_BOOL) {
        return value->data.bool_val;
    }
    return 0;
}

/**
 * Получение числового значения
 */
double json_get_number(json_value_t *value) {
    if (value && value->type == JSON_NUMBER) {
        return value->data.number_val;
    }
    return 0.0;
}

/**
 * Получение строкового значения
 */
const char* json_get_string(json_value_t *value) {
    if (value && value->type == JSON_STRING) {
        return value->data.string_val;
    }
    return NULL;
}

/**
 * Пропуск пробелов в JSON строке
 */
static const char* skip_whitespace(const char *str) {
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) {
        str++;
    }
    return str;
}

/**
 * Парсинг строки из JSON
 */
static const char* parse_string(const char *str, char **result) {
    if (*str != '"') return NULL;
    str++; // пропускаем открывающую кавычку
    
    const char *start = str;
    while (*str && *str != '"') {
        if (*str == '\\') str++; // пропускаем экранированный символ
        if (*str) str++;
    }
    
    if (*str != '"') return NULL;
    
    int len = str - start;
    *result = malloc(len + 1);
    strncpy(*result, start, len);
    (*result)[len] = '\0';
    
    return str + 1; // пропускаем закрывающую кавычку
}

/**
 * Парсинг числа из JSON
 */
static const char* parse_number(const char *str, double *result) {
    char *end;
    *result = strtod(str, &end);
    return (end > str) ? end : NULL;
}

/**
 * Предварительное объявление для взаимной рекурсии
 */
static const char* parse_value(const char *str, json_value_t *value);

/**
 * Парсинг объекта из JSON
 */
static const char* parse_object(const char *str, json_value_t *value) {
    if (*str != '{') return NULL;
    str++;
    
    value->type = JSON_OBJECT;
    value->data.object.count = 0;
    value->data.object.capacity = 16;
    value->data.object.keys = malloc(value->data.object.capacity * sizeof(char*));
    value->data.object.values = malloc(value->data.object.capacity * sizeof(json_value_t));
    
    str = skip_whitespace(str);
    
    if (*str == '}') {
        return str + 1; // пустой объект
    }
    
    while (*str) {
        str = skip_whitespace(str);
        
        // Парсим ключ
        char *key;
        str = parse_string(str, &key);
        if (!str) break;
        
        str = skip_whitespace(str);
        if (*str != ':') {
            free(key);
            break;
        }
        str++;
        
        str = skip_whitespace(str);
        
        // Парсим значение
        json_value_t val;
        str = parse_value(str, &val);
        if (!str) {
            free(key);
            break;
        }
        
        // Добавляем в объект
        if (value->data.object.count >= value->data.object.capacity) {
            value->data.object.capacity *= 2;
            value->data.object.keys = realloc(value->data.object.keys, 
                value->data.object.capacity * sizeof(char*));
            value->data.object.values = realloc(value->data.object.values,
                value->data.object.capacity * sizeof(json_value_t));
        }
        
        value->data.object.keys[value->data.object.count] = key;
        value->data.object.values[value->data.object.count] = val;
        value->data.object.count++;
        
        str = skip_whitespace(str);
        if (*str == '}') {
            return str + 1;
        } else if (*str == ',') {
            str++;
        } else {
            break;
        }
    }
    
    return NULL;
}

/**
 * Парсинг значения из JSON
 */
static const char* parse_value(const char *str, json_value_t *value) {
    str = skip_whitespace(str);
    
    if (*str == '"') {
        // Строка
        char *string_val;
        str = parse_string(str, &string_val);
        if (str) {
            value->type = JSON_STRING;
            value->data.string_val = string_val;
        }
        return str;
    } else if (*str == '{') {
        // Объект
        return parse_object(str, value);
    } else if (*str == 't' && strncmp(str, "true", 4) == 0) {
        // true
        value->type = JSON_BOOL;
        value->data.bool_val = 1;
        return str + 4;
    } else if (*str == 'f' && strncmp(str, "false", 5) == 0) {
        // false
        value->type = JSON_BOOL;
        value->data.bool_val = 0;
        return str + 5;
    } else if (*str == 'n' && strncmp(str, "null", 4) == 0) {
        // null
        value->type = JSON_NULL;
        return str + 4;
    } else if (*str == '-' || (*str >= '0' && *str <= '9')) {
        // Число
        double number_val;
        str = parse_number(str, &number_val);
        if (str) {
            value->type = JSON_NUMBER;
            value->data.number_val = number_val;
        }
        return str;
    }
    
    return NULL;
}

/**
 * Простой JSON парсер
 */
json_value_t* json_parse(const char *json_string) {
    if (!json_string) return NULL;
    
    json_value_t *result = malloc(sizeof(json_value_t));
    if (!result) return NULL;
    
    const char *end = parse_value(json_string, result);
    if (!end) {
        free(result);
        return NULL;
    }
    
    return result;
}

/**
 * Сериализация JSON в строку
 */
char* json_stringify(json_value_t *value) {
    if (!value) return NULL;
    
    char *result = malloc(4096); // Буфер для результата
    if (!result) return NULL;
    
    result[0] = '\0';
    
    switch (value->type) {
        case JSON_NULL:
            strcpy(result, "null");
            break;
            
        case JSON_BOOL:
            strcpy(result, value->data.bool_val ? "true" : "false");
            break;
            
        case JSON_NUMBER:
            snprintf(result, 4096, "%.2f", value->data.number_val);
            break;
            
        case JSON_STRING:
            snprintf(result, 4096, "\"%s\"", value->data.string_val);
            break;
            
        case JSON_ARRAY:
            strcpy(result, "[");
            for (int i = 0; i < value->data.array.count; i++) {
                if (i > 0) strcat(result, ",");
                char *item_str = json_stringify(&value->data.array.items[i]);
                if (item_str) {
                    strcat(result, item_str);
                    free(item_str);
                }
            }
            strcat(result, "]");
            break;
            
        case JSON_OBJECT:
            strcpy(result, "{");
            for (int i = 0; i < value->data.object.count; i++) {
                if (i > 0) strcat(result, ",");
                char key_str[256];
                snprintf(key_str, sizeof(key_str), "\"%s\":", value->data.object.keys[i]);
                strcat(result, key_str);
                
                char *val_str = json_stringify(&value->data.object.values[i]);
                if (val_str) {
                    strcat(result, val_str);
                    free(val_str);
                }
            }
            strcat(result, "}");
            break;
    }
    
    return result;
}

/**
 * Освобождение памяти JSON значения
 */
void json_free(json_value_t *value) {
    if (!value) return;
    
    switch (value->type) {
        case JSON_STRING:
            if (value->data.string_val) {
                free(value->data.string_val);
            }
            break;
            
        case JSON_ARRAY:
            for (int i = 0; i < value->data.array.count; i++) {
                json_free(&value->data.array.items[i]);
            }
            if (value->data.array.items) {
                free(value->data.array.items);
            }
            break;
            
        case JSON_OBJECT:
            for (int i = 0; i < value->data.object.count; i++) {
                if (value->data.object.keys[i]) {
                    free(value->data.object.keys[i]);
                }
                json_free(&value->data.object.values[i]);
            }
            if (value->data.object.keys) {
                free(value->data.object.keys);
            }
            if (value->data.object.values) {
                free(value->data.object.values);
            }
            break;
            
        default:
            break;
    }
    
    // Не освобождаем саму структуру если она не была выделена отдельно
}

/**
 * Проверка типов JSON значений
 */
int json_is_null(json_value_t *value) {
    return value && value->type == JSON_NULL;
}

int json_is_bool(json_value_t *value) {
    return value && value->type == JSON_BOOL;
}

int json_is_number(json_value_t *value) {
    return value && value->type == JSON_NUMBER;
}

int json_is_string(json_value_t *value) {
    return value && value->type == JSON_STRING;
}

int json_is_array(json_value_t *value) {
    return value && value->type == JSON_ARRAY;
}

int json_is_object(json_value_t *value) {
    return value && value->type == JSON_OBJECT;
}