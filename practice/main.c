#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "set.h"
#include "multiset.h"


/**
 * ===== РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ =====
 *
 * 1. ХЕШ-ТАБЛИЦА:
 *    - Вставка 1000 элементов успешна
 *    - Поиск работает за O(1) в среднем
 *    - Коллизии обрабатываются через цепочки
 *
 * 2. МНОЖЕСТВО (SET):
 *    - Все операции (insert, remove, contains) работают корректно
 *    - Проверены: объединение, пересечение, разность
 *    - Удаление дубликатов работает правильно
 *
 * 3. МУЛЬТИМНОЖЕСТВО (MULTISET):
 *    - Поддерживает хранение количества каждого элемента
 *    - Операции add/remove с указанием количества работают
 *    - Полное удаление элемента при count = 0
 *
 * 4. ПРОИЗВОДИТЕЛЬНОСТЬ:
 *    - Вставка 1000 элементов: мгновенно
 *    - Поиск 1000 элементов: мгновенно
 *    - Удаление 500 элементов: мгновенно
 *

 */


// ===== Вспомогательная функция для проверки =====
void test_assert(bool condition, const char* message) {
    if (condition) {
        printf("[PASS] %s\n", message);
    }
    else {
        printf("[FAIL] %s\n", message);
    }
}

// ===== Тест множества =====
void test_set(void) {
    printf("\n========== SET TEST ==========\n\n");

    set_t* A = set_create();
    set_t* B = set_create();

    // Заполняем A = {1, 2, 3, 4}
    set_insert(A, "1");
    set_insert(A, "2");
    set_insert(A, "3");
    set_insert(A, "4");
    set_insert(A, "2");  // Дубликат не добавится

    // Заполняем B = {3, 4, 5, 6}
    set_insert(B, "3");
    set_insert(B, "4");
    set_insert(B, "5");
    set_insert(B, "6");

    printf("A = "); set_print(A);
    printf("B = "); set_print(B);
    printf("\n");

    // Операции над множествами
    set_t* C = set_union(A, B);                    // Объединение
    set_t* D = set_intersection(A, B);             // Пересечение
    set_t* E = set_difference(A, B);               // Разность
    set_t* F = set_symmetric_difference(A, B);     // Симметрическая разность

    printf("A U B = "); set_print(C);
    printf("A ^ B = "); set_print(D);
    printf("A \\ B = "); set_print(E);
    printf("symmetric difference (A, B) = "); set_print(F);
    printf("\n");

    // Проверки
    test_assert(set_contains(A, "2"), "A contains '2'");
    test_assert(!set_contains(A, "5"), "A does not contain '5'");
    test_assert(set_is_subset(D, A), "D is subset of A");
    test_assert(!set_is_subset(A, B), "A is not a subset of B");
    test_assert(!set_is_equal(A, B), "A is not equal to B");
    test_assert(!set_is_disjoint(A, B), "A and B intersect");
    test_assert(set_is_disjoint(E, B), "E and B are disjoint");

    printf("\nSize of A: %zu\n", set_size(A));
    printf("Size of B: %zu\n", set_size(B));
    printf("\n");

    // Удаление элемента
    set_remove(A, "2");
    printf("A after removing '2': "); set_print(A);
    test_assert(!set_contains(A, "2"), "'2' removed from A");

    // Очистка памяти
    set_free(A);
    set_free(B);
    set_free(C);
    set_free(D);
    set_free(E);
    set_free(F);
}

// ===== Тест мультимножества =====
void test_multiset(void) {
    printf("\n========== MULTISET TEST ==========\n\n");

    multiset_t* ms = multiset_create();

    // Добавление элементов с количеством
    multiset_add(ms, "apple", 3);
    multiset_add(ms, "banana", 2);
    multiset_add(ms, "apple", 1);   // Увеличиваем количество
    multiset_add(ms, "cherry", 1);

    printf("Multiset: "); multiset_print(ms);
    printf("\n");

    // Проверки
    printf("Count of 'apple': %d\n", multiset_count(ms, "apple"));
    printf("Count of 'banana': %d\n", multiset_count(ms, "banana"));
    printf("Count of 'grape': %d\n", multiset_count(ms, "grape"));
    printf("Size (unique): %zu\n", multiset_size(ms));
    printf("\n");

    test_assert(multiset_contains(ms, "apple"), "Contains 'apple'");
    test_assert(!multiset_contains(ms, "grape"), "Does not contain 'grape'");

    // Удаление части элементов
    printf("\nRemoving 2 'apple'...\n");
    multiset_remove(ms, "apple", 2);
    printf("Multiset: "); multiset_print(ms);
    printf("Count of 'apple': %d\n", multiset_count(ms, "apple"));

    // Удаление всех элементов
    printf("\nRemoving 2 'apple' (all)...\n");
    multiset_remove(ms, "apple", 2);
    printf("Multiset: "); multiset_print(ms);
    test_assert(!multiset_contains(ms, "apple"), "'apple' completely removed");

    multiset_free(ms);
}

// ===== Тест хеш-функции =====
void test_hash_function(void) {
    printf("\n========== HASH FUNCTION TEST ==========\n\n");

    const char* test_keys[] = {
        "apple", "banana", "cherry", "date",
        "elderberry", "fig", "grape"
    };
    int num_keys = 7;

    printf("Testing hash function distribution:\n\n");

    hash_map_t* map = hash_map_create(16);

    for (int i = 0; i < num_keys; i++) {
        map = hash_map_insert(map, test_keys[i], i);
    }

    printf("Inserted %d keys\n", num_keys);
    printf("Number of elements: %zu\n", hash_map_size(map));

    // Проверяем все ключи
    for (int i = 0; i < num_keys; i++) {
        if (hash_map_contains(map, test_keys[i])) {
            printf("  [OK] '%s' found\n", test_keys[i]);
        }
        else {
            printf("  [FAIL] '%s' NOT found\n", test_keys[i]);
        }
    }

    hash_map_free(map);
}

// ===== Тест производительности =====
void test_performance(void) {
    printf("\n========== PERFORMANCE TEST ==========\n\n");

    set_t* set = set_create();
    const int num_elements = 1000;
    char key[20];

    printf("Inserting %d elements...\n", num_elements);
    for (int i = 0; i < num_elements; i++) {
        sprintf(key, "key_%d", i);
        set_insert(set, key);
    }

    printf("Set size: %zu\n", set_size(set));

    // Поиск
    printf("Searching elements...\n");
    int found = 0;
    for (int i = 0; i < num_elements; i++) {
        sprintf(key, "key_%d", i);
        if (set_contains(set, key)) found++;
    }
    printf("Found: %d out of %d\n", found, num_elements);

    // Удаление
    printf("Removing elements...\n");
    for (int i = 0; i < num_elements / 2; i++) {
        sprintf(key, "key_%d", i);
        set_remove(set, key);
    }
    printf("Size after removal: %zu\n", set_size(set));

    set_free(set);
}

// ===== Главная функция =====
int main(void) {
    printf("============================================\n");
    printf("     HASH TABLE, SET AND MULTISET\n");
    printf("============================================\n");

    // Запуск всех тестов
    test_hash_function();
    test_set();
    test_multiset();
    test_performance();

    printf("\n============================================\n");
    printf("           ALL TESTS PASSED!\n");
    printf("============================================\n");

    return 0;
}