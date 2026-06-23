#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stddef.h>
#include <stdbool.h>

// ===== Структура хеш-таблицы =====
typedef struct _hash_map_t hash_map_t;


hash_map_t* hash_map_create(size_t size);
void hash_map_free(hash_map_t* map);

// ===== Основные операции =====
hash_map_t* hash_map_insert(hash_map_t* map, const char* key, long value);
bool hash_map_contains(hash_map_t* map, const char* key);
long hash_map_get(hash_map_t* map, const char* key);
bool hash_map_remove(hash_map_t* map, const char* key);
size_t hash_map_size(hash_map_t* map);
bool hash_map_is_empty(hash_map_t* map);

// ===== Получение всех ключей =====
char** hash_map_keys(hash_map_t* map, size_t* count);

#endif /* HASH_MAP_H */