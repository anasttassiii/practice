#ifndef MULTISET_H
#define MULTISET_H

#include <stddef.h>
#include <stdbool.h>
#include "hashmap.h"

// ===== Мультимножество =====
typedef struct _multiset_t {
    hash_map_t* map;
} multiset_t;


multiset_t* multiset_create(void);
void multiset_free(multiset_t* ms);

// ===== Основные операции =====
bool multiset_add(multiset_t* ms, const char* element, int count);
bool multiset_remove(multiset_t* ms, const char* element, int count);
int multiset_count(multiset_t* ms, const char* element);
bool multiset_contains(multiset_t* ms, const char* element);
size_t multiset_size(multiset_t* ms);
bool multiset_is_empty(multiset_t* ms);


void multiset_print(multiset_t* ms);

#endif /* MULTISET_H */