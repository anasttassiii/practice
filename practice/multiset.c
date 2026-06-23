#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "multiset.h"

// ===== Создание мультимножества =====
multiset_t* multiset_create(void) {
    multiset_t* ms = malloc(sizeof(multiset_t));
    if (ms == NULL) return NULL;

    ms->map = hash_map_create(16);
    if (ms->map == NULL) {
        free(ms);
        return NULL;
    }

    return ms;
}

// ===== Освобождение памяти =====
void multiset_free(multiset_t* ms) {
    if (ms == NULL) return;
    hash_map_free(ms->map);
    free(ms);
}

// ===== Добавление элемента =====
bool multiset_add(multiset_t* ms, const char* element, int count) {
    assert(ms != NULL);
    assert(element != NULL);
    assert(count > 0);

    int current = 0;
    if (hash_map_contains(ms->map, element)) {
        current = (int)hash_map_get(ms->map, element);
    }

    ms->map = hash_map_insert(ms->map, element, current + count);
    return true;
}

// ===== Удаление элемента =====
bool multiset_remove(multiset_t* ms, const char* element, int count) {
    assert(ms != NULL);
    assert(element != NULL);
    assert(count > 0);

    if (!hash_map_contains(ms->map, element)) {
        return false;
    }

    int current = (int)hash_map_get(ms->map, element);

    if (current <= count) {
        return hash_map_remove(ms->map, element);
    }
    else {
        ms->map = hash_map_insert(ms->map, element, current - count);
        return true;
    }
}

// ===== Получение количества =====
int multiset_count(multiset_t* ms, const char* element) {
    assert(ms != NULL);
    assert(element != NULL);

    if (!hash_map_contains(ms->map, element)) {
        return 0;
    }

    return (int)hash_map_get(ms->map, element);
}

// ===== Проверка наличия =====
bool multiset_contains(multiset_t* ms, const char* element) {
    assert(ms != NULL);
    assert(element != NULL);
    return hash_map_contains(ms->map, element);
}

// ===== Размер =====
size_t multiset_size(multiset_t* ms) {
    assert(ms != NULL);
    return hash_map_size(ms->map);
}

bool multiset_is_empty(multiset_t* ms) {
    assert(ms != NULL);
    return hash_map_is_empty(ms->map);
}

// ===== Вывод =====
void multiset_print(multiset_t* ms) {
    assert(ms != NULL);

    printf("{ ");
    size_t count;
    char** keys = hash_map_keys(ms->map, &count);

    for (size_t i = 0; i < count; i++) {
        int cnt = (int)hash_map_get(ms->map, keys[i]);
        printf("%s: %d", keys[i], cnt);
        if (i < count - 1) printf(", ");
        free(keys[i]);
    }
    free(keys);

    printf(" }\n");
}