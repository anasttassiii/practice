#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include "hashmap.h"

// ===== СТРУКТУРА ЗАПИСИ (атомарные поля) =====
typedef struct _hash_map_entry_t {
    volatile uintptr_t key;      // 0 = пусто, иначе указатель на строку
    volatile long value;
    volatile bool deleted;
} hash_map_entry_t;

// ===== СТРУКТУРА ХЕШ-ТАБЛИЦЫ =====
struct _hash_map_t {
    hash_map_entry_t* entries;
    size_t size;
    size_t mask;          // size - 1 (для быстрого modulo)
    volatile long count;  // атомарный счетчик
};

// ===== АТОМАРНЫЕ ОПЕРАЦИИ через Interlocked =====

// Загрузка ключа
static inline uintptr_t atomic_load_key(volatile uintptr_t* ptr) {
    return (uintptr_t)InterlockedCompareExchangePointer((volatile PVOID*)ptr, NULL, NULL);
}

// Сохранение ключа
static inline void atomic_store_key(volatile uintptr_t* ptr, uintptr_t value) {
    InterlockedExchangePointer((volatile PVOID*)ptr, (PVOID)value);
}

// CAS для ключа (Compare-And-Swap)
static inline bool atomic_cas_key(volatile uintptr_t* ptr, uintptr_t expected, uintptr_t desired) {
    return InterlockedCompareExchangePointer((volatile PVOID*)ptr, (PVOID)desired, (PVOID)expected) == (PVOID)expected;
}

// Загрузка значения
static inline long atomic_load_value(volatile long* ptr) {
    return InterlockedCompareExchange((volatile LONG*)ptr, 0, 0);
}

// Сохранение значения
static inline void atomic_store_value(volatile long* ptr, long value) {
    InterlockedExchange((volatile LONG*)ptr, (LONG)value);
}

// Загрузка флага deleted
static inline bool atomic_load_deleted(volatile bool* ptr) {
    return (bool)InterlockedCompareExchange((volatile LONG*)ptr, 0, 0);
}

// Сохранение флага deleted
static inline void atomic_store_deleted(volatile bool* ptr, bool value) {
    InterlockedExchange((volatile LONG*)ptr, (LONG)value);
}

// Атомарное увеличение/уменьшение счетчика
static inline long atomic_fetch_add_count(volatile long* ptr, long value) {
    return InterlockedAdd((volatile LONG*)ptr, (LONG)value) - value;
}

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
    if (expanded == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for hash table expansion\n");
        return NULL;
    }

    // Перехешируем все существующие элементы
    for (size_t i = 0; i < map->size; i++) {
        uintptr_t key_ptr = atomic_load_key(&map->entries[i].key);
        if (key_ptr != 0) {
            bool deleted = atomic_load_deleted(&map->entries[i].deleted);
            if (!deleted) {
                const char* key = (const char*)key_ptr;
                long value = atomic_load_value(&map->entries[i].value);
                expanded = hash_map_insert(expanded, key, value);
                if (expanded == NULL) {
                    fprintf(stderr, "ERROR: Failed to rehash during expansion\n");
                    hash_map_free(map);
                    return NULL;
                }
            }
        }
    }

    hash_map_free(map);
    return expanded;
}

// ===== Создание таблицы =====
hash_map_t* hash_map_create(size_t size) {
    if (size == 0) size = 1;

    // Размер должен быть степенью двойки
    size_t power = 1;
    while (power < size) power <<= 1;

    hash_map_t* map = malloc(sizeof(hash_map_t));
    if (map == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for hash map\n");
        return NULL;
    }

    map->size = power;
    map->mask = power - 1;
    map->count = 0;
    map->entries = calloc(map->size, sizeof(hash_map_entry_t));

    if (map->entries == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for entries\n");
        free(map);
        return NULL;
    }

    return map;
}

// ===== Вставка элемента (Lock-Free) =====
hash_map_t* hash_map_insert(hash_map_t* map, const char* key, long value) {
    assert(map != NULL);
    assert(key != NULL);

    uint32_t hash = hash_function(key);
    size_t idx = hash & map->mask;
    size_t start_idx = idx;

    // 1. СНАЧАЛА проверяем наличие ключа (без расширения!)
    do {
        uintptr_t key_ptr = atomic_load_key(&map->entries[idx].key);
        bool deleted = atomic_load_deleted(&map->entries[idx].deleted);

        if (key_ptr != 0 && !deleted) {
            const char* existing_key = (const char*)key_ptr;
            if (strcmp(existing_key, key) == 0) {
                // Обновляем значение
                atomic_store_value(&map->entries[idx].value, value);
                return map;
            }
        }

        idx = (idx + 1) & map->mask;
    } while (idx != start_idx);

    // 2. Расширение ТОЛЬКО после проверки наличия
    if (map->count >= (long)(map->size * 0.75)) {
        map = hash_map_expand(map);
        if (map == NULL) {
            fprintf(stderr, "ERROR: Failed to expand hash table\n");
            return NULL;
        }
        // Повторяем вставку в расширенной таблице
        return hash_map_insert(map, key, value);
    }

    // 3. Вставка нового элемента (Lock-Free через CAS)
    idx = hash & map->mask;
    size_t first_deleted = map->size;
    start_idx = idx;

    do {
        uintptr_t key_ptr = atomic_load_key(&map->entries[idx].key);
        bool deleted = atomic_load_deleted(&map->entries[idx].deleted);

        // Запоминаем первую удаленную ячейку
        if (deleted && first_deleted == map->size) {
            first_deleted = idx;
        }

        // Если ячейка пуста (не удалена)
        if (key_ptr == 0 && !deleted) {
            // Используем первую найденную удаленную
            if (first_deleted != map->size) {
                idx = first_deleted;
            }

            // Копируем ключ
            char* new_key = calloc(strlen(key) + 1, sizeof(char));
            if (new_key == NULL) {
                fprintf(stderr, "ERROR: Failed to allocate memory for key\n");
                return map;
            }
            strcpy(new_key, key);

            // Пытаемся атомарно захватить ячейку через CAS
            if (atomic_cas_key(&map->entries[idx].key, 0, (uintptr_t)new_key)) {
                // Захватили! Пишем значение
                atomic_store_value(&map->entries[idx].value, value);
                atomic_store_deleted(&map->entries[idx].deleted, false);
                atomic_fetch_add_count(&map->count, 1);
                return map;
            }
            else {
                // CAS не удался — другой поток занял ячейку
                free(new_key);
                // Продолжаем поиск
            }
        }

        idx = (idx + 1) & map->mask;
    } while (idx != start_idx);

    // Если таблица полна (расширение должно было сработать)
    fprintf(stderr, "WARNING: Hash table is full, expanding...\n");
    map = hash_map_expand(map);
    if (map == NULL) return NULL;
    return hash_map_insert(map, key, value);
}

// ===== Проверка наличия ключа (Lock-Free) =====
bool hash_map_contains(hash_map_t* map, const char* key) {
    assert(map != NULL);
    assert(key != NULL);

    uint32_t hash = hash_function(key);
    size_t idx = hash & map->mask;
    size_t start_idx = idx;

    do {
        uintptr_t key_ptr = atomic_load_key(&map->entries[idx].key);
        bool deleted = atomic_load_deleted(&map->entries[idx].deleted);

        if (key_ptr != 0 && !deleted) {
            const char* existing_key = (const char*)key_ptr;
            if (strcmp(existing_key, key) == 0) {
                return true;
            }
        }

        idx = (idx + 1) & map->mask;
    } while (idx != start_idx);

    return false;
}

// ===== Получение значения (Lock-Free) =====
long hash_map_get(hash_map_t* map, const char* key) {
    assert(map != NULL);
    assert(key != NULL);

    uint32_t hash = hash_function(key);
    size_t idx = hash & map->mask;
    size_t start_idx = idx;

    do {
        uintptr_t key_ptr = atomic_load_key(&map->entries[idx].key);
        bool deleted = atomic_load_deleted(&map->entries[idx].deleted);

        if (key_ptr != 0 && !deleted) {
            const char* existing_key = (const char*)key_ptr;
            if (strcmp(existing_key, key) == 0) {
                return atomic_load_value(&map->entries[idx].value);
            }
        }

        idx = (idx + 1) & map->mask;
    } while (idx != start_idx);

    return 0;
}

// ===== Удаление элемента (Lock-Free) =====
bool hash_map_remove(hash_map_t* map, const char* key) {
    assert(map != NULL);
    assert(key != NULL);

    uint32_t hash = hash_function(key);
    size_t idx = hash & map->mask;
    size_t start_idx = idx;

    do {
        uintptr_t key_ptr = atomic_load_key(&map->entries[idx].key);
        bool deleted = atomic_load_deleted(&map->entries[idx].deleted);

        if (key_ptr != 0 && !deleted) {
            const char* existing_key = (const char*)key_ptr;
            if (strcmp(existing_key, key) == 0) {
                // Помечаем как удаленное
                atomic_store_deleted(&map->entries[idx].deleted, true);
                // Освобождаем память ключа
                free((void*)key_ptr);
                atomic_store_key(&map->entries[idx].key, 0);
                atomic_fetch_add_count(&map->count, -1);
                return true;
            }
        }

        idx = (idx + 1) & map->mask;
    } while (idx != start_idx);

    return false;
}

// ===== Размер =====
size_t hash_map_size(hash_map_t* map) {
    assert(map != NULL);
    return (size_t)map->count;
}

bool hash_map_is_empty(hash_map_t* map) {
    assert(map != NULL);
    return map->count == 0;
}

// ===== Получение всех ключей =====
char** hash_map_keys(hash_map_t* map, size_t* count) {
    assert(map != NULL);
    assert(count != NULL);

    *count = (size_t)map->count;
    if (*count == 0) return NULL;

    char** keys = malloc(*count * sizeof(char*));
    if (keys == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for keys\n");
        return NULL;
    }

    size_t idx_out = 0;
    for (size_t i = 0; i < map->size && idx_out < *count; i++) {
        uintptr_t key_ptr = atomic_load_key(&map->entries[i].key);
        bool deleted = atomic_load_deleted(&map->entries[i].deleted);

        if (key_ptr != 0 && !deleted) {
            const char* key = (const char*)key_ptr;
            keys[idx_out] = malloc(strlen(key) + 1);
            if (keys[idx_out] != NULL) {
                strcpy(keys[idx_out], key);
            }
            idx_out++;
        }
    }

    return keys;
}

// ===== Освобождение памяти =====
void hash_map_free(hash_map_t* map) {
    if (map == NULL) return;

    for (size_t i = 0; i < map->size; i++) {
        uintptr_t key_ptr = atomic_load_key(&map->entries[i].key);
        if (key_ptr != 0) {
            free((void*)key_ptr);
        }
    }

    free(map->entries);
    free(map);
}