#ifndef SET_H
#define SET_H

#include <stddef.h>
#include <stdbool.h>
#include "hashmap.h"

// ===== Множество =====
typedef struct _set_t {
    hash_map_t* map;
} set_t;


set_t* set_create(void);
void set_free(set_t* set);

// ===== Основные операции =====
bool set_insert(set_t* set, const char* element);
bool set_contains(set_t* set, const char* element);
bool set_remove(set_t* set, const char* element);
size_t set_size(set_t* set);
bool set_is_empty(set_t* set);

// ===== Операции над множествами =====
set_t* set_union(set_t* a, set_t* b);
set_t* set_intersection(set_t* a, set_t* b);
set_t* set_difference(set_t* a, set_t* b);
set_t* set_symmetric_difference(set_t* a, set_t* b);
bool set_is_subset(set_t* a, set_t* b);
bool set_is_equal(set_t* a, set_t* b);
bool set_is_disjoint(set_t* a, set_t* b);


void set_print(set_t* set);
char** set_to_array(set_t* set, size_t* count);

#endif /* SET_H */