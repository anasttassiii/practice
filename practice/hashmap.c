#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h> 
#include "hashmap.h"


 // ===== СТРУКТУРА ЗАПИСИ =====
typedef struct _hash_map_entry_t {
    char* key;
    long value;
    bool deleted;
} hash_map_entry_t;

// ===== СТРУКТУРА ХЕШ-ТАБЛИЦЫ =====
struct _hash_map_t {
    hash_map_entry_t* entries;  
    size_t size;
    size_t count;
};

// ===== Хеш-функция (MurmurHash3 - упрощенная версия) =====
static uint32_t hash_function(const char* key) {  
    assert(key != NULL);

    uint32_t hash = 0x12345678; 

    for (const char* c = key; *c != '\0'; c++) {
        hash ^= (uint32_t)*c;
        hash *= 0x5bd1e995;
        hash ^= hash >> 15;
    }

    return hash;
}

// ===== Расширение таблицы =====
static hash_map_t* hash_map_expand(hash_map_t* map) {
    assert(map != NULL);

    hash_map_t* expanded = hash_map_create(map->size * 2);
    if (expanded == NULL) return NULL;

    for (size_t i = 0; i < map->size; i++) {
        if (map->entries[i].key != NULL && !map->entries[i].deleted) {
            expanded = hash_map_insert(expanded, map->entries[i].key, map->entries[i].value);
        }
    }

    hash_map_free(map);
    return expanded;
}

// ===== Создание таблицы =====
hash_map_t* hash_map_create(size_t size) {
    if (size == 0) size = 1;

    hash_map_t* map = malloc(sizeof(hash_map_t));
    if (map == NULL) return NULL;

    map->size = size;
    map->count = 0;
    map->entries = calloc(map->size, sizeof(hash_map_entry_t));

    if (map->entries == NULL) {
        free(map);
        return NULL;
    }

    return map;
}

// ===== Вставка элемента =====
hash_map_t* hash_map_insert(hash_map_t* map, const char* key, long value) {
    assert(map != NULL);
    assert(key != NULL);

    // Расширение при нагрузке > 0.75
    if (map->count >= map->size * 0.75) {
        map = hash_map_expand(map);
        if (map == NULL) return NULL;
    }

    size_t start = hash_function(key) % map->size;
    size_t idx = start;
    size_t first_deleted = map->size;
    size_t step = 1;

    // Линейное пробирование с циклическим обходом
    do {
        // Проверяем существующий ключ
        if (map->entries[idx].key != NULL && !map->entries[idx].deleted) {
            if (strcmp(map->entries[idx].key, key) == 0) {
                map->entries[idx].value = value;  // Обновление
                return map;
            }
        }

        // Запоминаем первую удаленную ячейку
        if (map->entries[idx].deleted && first_deleted == map->size) {
            first_deleted = idx;
        }

        // Нашли пустую ячейку
        if (map->entries[idx].key == NULL && !map->entries[idx].deleted) {
            if (first_deleted != map->size) {
                idx = first_deleted;
            }

        
            char* new_key = calloc(strlen(key) + 1, sizeof(char));
            if (new_key == NULL) {
                return map;  // Не удалось выделить память
            }

            strcpy(new_key, key);
            map->entries[idx].key = new_key;
            map->entries[idx].value = value;
            map->entries[idx].deleted = false;
            map->count++;
            return map;
        }

        idx = (idx + 1) % map->size;
        step++;

        if (step > map->size) {
            map = hash_map_expand(map);
            if (map == NULL) return NULL;
            return hash_map_insert(map, key, value);
        }
    } while (idx != start);

    return map;
}

// ===== Проверка наличия ключа =====
bool hash_map_contains(hash_map_t* map, const char* key) {
    assert(map != NULL);
    assert(key != NULL);

    size_t start = hash_function(key) % map->size;
    size_t idx = start;

    do {
        if (map->entries[idx].key != NULL && !map->entries[idx].deleted) {
            if (strcmp(map->entries[idx].key, key) == 0) {
                return true;
            }
        }
        idx = (idx + 1) % map->size;
    } while (idx != start);

    return false;
}

// ===== Получение значения =====
long hash_map_get(hash_map_t* map, const char* key) {
    assert(map != NULL);
    assert(key != NULL);

    size_t start = hash_function(key) % map->size;
    size_t idx = start;

    do {
        if (map->entries[idx].key != NULL && !map->entries[idx].deleted) {
            if (strcmp(map->entries[idx].key, key) == 0) {
                return map->entries[idx].value;
            }
        }
        idx = (idx + 1) % map->size;
    } while (idx != start);

    return 0;
}


bool hash_map_remove(hash_map_t* map, const char* key) {
    assert(map != NULL);
    assert(key != NULL);

    size_t start = hash_function(key) % map->size;
    size_t idx = start;

    do {
        if (map->entries[idx].key != NULL && !map->entries[idx].deleted) {
            if (strcmp(map->entries[idx].key, key) == 0) {
                free(map->entries[idx].key);
                map->entries[idx].key = NULL;
                map->entries[idx].deleted = true;
                map->count--;
                return true;
            }
        }
        idx = (idx + 1) % map->size;
    } while (idx != start);

    return false;
}


size_t hash_map_size(hash_map_t* map) {
    assert(map != NULL);
    return map->count;
}

bool hash_map_is_empty(hash_map_t* map) {
    assert(map != NULL);
    return map->count == 0;
}

// ===== Получение всех ключей =====
char** hash_map_keys(hash_map_t* map, size_t* count) {
    assert(map != NULL);
    assert(count != NULL);

    *count = map->count;
    if (*count == 0) return NULL;

    char** keys = malloc(*count * sizeof(char*));
    if (keys == NULL) return NULL;

    size_t idx = 0;
    for (size_t i = 0; i < map->size && idx < *count; i++) {
        if (map->entries[i].key != NULL && !map->entries[i].deleted) {
            keys[idx] = malloc(strlen(map->entries[i].key) + 1);
            if (keys[idx] != NULL) {
                strcpy(keys[idx], map->entries[i].key);
            }
            idx++;
        }
    }

    return keys;
}


void hash_map_free(hash_map_t* map) {
    if (map == NULL) return;

    for (size_t i = 0; i < map->size; i++) {
        if (map->entries[i].key != NULL) {
            free(map->entries[i].key);
        }
    }

    free(map->entries);
    free(map);
}