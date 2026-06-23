#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "set.h"

// ===== Создание множества =====
set_t* set_create(void) {
    set_t* set = malloc(sizeof(set_t));
    if (set == NULL) return NULL;

    set->map = hash_map_create(16);
    if (set->map == NULL) {
        free(set);
        return NULL;
    }

    return set;
}

// ===== Освобождение памяти =====
void set_free(set_t* set) {
    if (set == NULL) return;
    hash_map_free(set->map);
    free(set);
}

// ===== Вставка элемента =====
bool set_insert(set_t* set, const char* element) {
    assert(set != NULL);
    assert(element != NULL);

    if (set_contains(set, element)) {
        return false;
    }

    set->map = hash_map_insert(set->map, element, 1);
    return true;
}

// ===== Проверка наличия элемента =====
bool set_contains(set_t* set, const char* element) {
    assert(set != NULL);
    assert(element != NULL);
    return hash_map_contains(set->map, element);
}

// ===== Удаление элемента =====
bool set_remove(set_t* set, const char* element) {
    assert(set != NULL);
    assert(element != NULL);
    return hash_map_remove(set->map, element);
}

// ===== Размер множества =====
size_t set_size(set_t* set) {
    assert(set != NULL);
    return hash_map_size(set->map);
}

bool set_is_empty(set_t* set) {
    assert(set != NULL);
    return hash_map_is_empty(set->map);
}

// ===== Объединение =====
set_t* set_union(set_t* a, set_t* b) {
    assert(a != NULL);
    assert(b != NULL);

    set_t* result = set_create();
    if (result == NULL) return NULL;

    size_t count_a;
    char** keys_a = hash_map_keys(a->map, &count_a);
    for (size_t i = 0; i < count_a; i++) {
        set_insert(result, keys_a[i]);
        free(keys_a[i]);
    }
    free(keys_a);

    size_t count_b;
    char** keys_b = hash_map_keys(b->map, &count_b);
    for (size_t i = 0; i < count_b; i++) {
        set_insert(result, keys_b[i]);
        free(keys_b[i]);
    }
    free(keys_b);

    return result;
}

// ===== Пересечение =====
set_t* set_intersection(set_t* a, set_t* b) {
    assert(a != NULL);
    assert(b != NULL);

    set_t* result = set_create();
    if (result == NULL) return NULL;

    set_t* smaller = (set_size(a) < set_size(b)) ? a : b;
    set_t* larger = (set_size(a) < set_size(b)) ? b : a;

    size_t count;
    char** keys = hash_map_keys(smaller->map, &count);
    for (size_t i = 0; i < count; i++) {
        if (set_contains(larger, keys[i])) {
            set_insert(result, keys[i]);
        }
        free(keys[i]);
    }
    free(keys);

    return result;
}

// ===== Разность =====
set_t* set_difference(set_t* a, set_t* b) {
    assert(a != NULL);
    assert(b != NULL);

    set_t* result = set_create();
    if (result == NULL) return NULL;

    size_t count;
    char** keys = hash_map_keys(a->map, &count);
    for (size_t i = 0; i < count; i++) {
        if (!set_contains(b, keys[i])) {
            set_insert(result, keys[i]);
        }
        free(keys[i]);
    }
    free(keys);

    return result;
}

// ===== Симметрическая разность =====
set_t* set_symmetric_difference(set_t* a, set_t* b) {
    assert(a != NULL);
    assert(b != NULL);

    set_t* diff1 = set_difference(a, b);
    set_t* diff2 = set_difference(b, a);
    set_t* result = set_union(diff1, diff2);

    set_free(diff1);
    set_free(diff2);

    return result;
}

// ===== A ⊆ B =====
bool set_is_subset(set_t* a, set_t* b) {
    assert(a != NULL);
    assert(b != NULL);

    if (set_size(a) > set_size(b)) return false;

    size_t count;
    char** keys = hash_map_keys(a->map, &count);
    for (size_t i = 0; i < count; i++) {
        if (!set_contains(b, keys[i])) {
            free(keys[i]);
            free(keys);
            return false;
        }
        free(keys[i]);
    }
    free(keys);

    return true;
}

// ===== A = B =====
bool set_is_equal(set_t* a, set_t* b) {
    assert(a != NULL);
    assert(b != NULL);
    return set_is_subset(a, b) && set_is_subset(b, a);
}

// ===== A ∩ B = ∅ =====
bool set_is_disjoint(set_t* a, set_t* b) {
    assert(a != NULL);
    assert(b != NULL);

    size_t count;
    char** keys = hash_map_keys(a->map, &count);
    for (size_t i = 0; i < count; i++) {
        if (set_contains(b, keys[i])) {
            free(keys[i]);
            free(keys);
            return false;
        }
        free(keys[i]);
    }
    free(keys);

    return true;
}

// ===== Вывод множества =====
void set_print(set_t* set) {
    assert(set != NULL);

    printf("{ ");
    size_t count;
    char** keys = hash_map_keys(set->map, &count);

    for (size_t i = 0; i < count; i++) {
        printf("%s", keys[i]);
        if (i < count - 1) printf(", ");
        free(keys[i]);
    }
    free(keys);

    printf(" }\n");
}

// ===== Преобразование в массив =====
char** set_to_array(set_t* set, size_t* count) {
    assert(set != NULL);
    assert(count != NULL);
    return hash_map_keys(set->map, count);
}