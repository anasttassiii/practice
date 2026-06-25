#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>      // Для CreateThread, WaitForMultipleObjects, HANDLE
#include "hashmap.h"
#include "set.h"
#include "multiset.h"


#define NUM_THREADS 4           // Количество потоков 
#define NUM_OPERATIONS 1000     // Сколько операций делает каждый поток


/**
 *  Структура, которая передается в каждый поток.
 *
 * Каждый поток получает:
 *   - map       -> указатель на хеш-таблицу (все потоки работают с одной таблицей)
 *   - thread_id -> уникальный номер потока (0, 1, 2, 3)
 */
typedef struct {
    hash_map_t* map;        // Указатель на хеш-таблицу
    int thread_id;          // Номер потока (0, 1, 2, 3)
} thread_data_t;

// ============================================================================
// ФУНКЦИЯ ПОТОКА-ПИСАТЕЛЯ
// ============================================================================

/**
 *  Функция, которую выполняет каждый поток-писатель.
 *   1. Получает данные (таблицу и свой номер)
 *   2. В цикле 1000 раз генерирует ключи вида "key_0_0", "key_0_1", ...
 *   3. Вставляет их в хеш-таблицу со значениями i * 10 + id
 *
 * Все 4 потока работают параллельно и не блокируют друг друга
 */
DWORD WINAPI writer_thread(LPVOID arg) {
    // 1. Превращаем сырые данные в структуру
    thread_data_t* data = (thread_data_t*)arg;
    hash_map_t* map = data->map;      // Таблица, куда пишем
    int id = data->thread_id;          // Номер потока

    char key[32];                      // Буфер для формирования ключа

    // 2. Вставляем NUM_OPERATIONS элементов
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        // Формируем ключ: "key_<номер_потока>_<номер_итерации>"
        sprintf(key, "key_%d_%d", id, i);

        // Вставляем в таблицу: ключ -> значение (i * 10 + id)
        // Значение разное для каждого потока, чтобы не было конфликтов
        map = hash_map_insert(map, key, i * 10 + id);
    }

    return 0;  
}

// ============================================================================
// ФУНКЦИЯ ПОТОКА-ЧИТАТЕЛЯ
// ============================================================================

/**
 * Функция, которую выполняет каждый поток-читатель.

 *   1. Получает данные (таблицу и свой номер)
 *   2. В цикле 1000 раз генерирует ключи вида "key_0_0", "key_0_1", ...
 *   3. Проверяет, есть ли такой ключ в таблице
 *   4. Считает количество найденных ключей
 *
 * Каждый читатель ищет только свои ключи (те, которые он сам вставил).
 * Это позволяет проверить, что все записи сохранились.
 *
 * Возвращает: сколько ключей найдено (ожидается 1000)
 */
DWORD WINAPI reader_thread(LPVOID arg) {
    // 1. Получаем данные
    thread_data_t* data = (thread_data_t*)arg;
    hash_map_t* map = data->map;
    int id = data->thread_id;

    int found = 0;                     // Счетчик найденных ключей
    char key[32];

    // 2. Проверяем все свои ключи
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        sprintf(key, "key_%d_%d", id, i);

        // Если ключ есть в таблице — увеличиваем счетчик
        if (hash_map_contains(map, key)) {
            found++;
        }
    }

    return (DWORD)found;  // Возвращаем количество найденных ключей
}

/**
 * Тест Lock-Free хеш-таблицы в многопоточной среде.
 *
 * Проверяет три сценария:
 *   1. Запись: 4 потока одновременно вставляют 4000 элементов
 *   2. Чтение: 4 потока одновременно читают 4000 элементов
 *   3. Удаление: удаление половины элементов
 *
 * Если все тесты проходят — таблица работает корректно и безопасно
 * в многопоточной среде без блокировок.
 */
void test_multithreaded(void) {
    printf("\n========== LOCK-FREE TEST ==========\n\n");

    // ===== 1. ПОДГОТОВКА =======

    // Всего элементов: 4 потока × 1000 операций = 4000
    size_t total_elements = NUM_THREADS * NUM_OPERATIONS;

    /**
     * Создаем таблицу заведомо большего размера, чтобы ИЗБЕЖАТЬ РАСШИРЕНИЯ
     * во время теста. Расширение — сложная операция, мы хотим проверить
     * именно работу Lock-Free вставки без дополнительных факторов.
     */
    size_t table_size = 1;
    while (table_size < total_elements * 2) {  // Пока < 8000
        table_size <<= 1;                       // Удваиваем
    }
    // table_size = 8192

    // Создаем хеш-таблицу
    hash_map_t* map = hash_map_create(table_size);
    if (map == NULL) {
        printf("ERROR: Failed to create hash map\n");
        return;
    }

    printf("Table size: %zu (elements: %zu)\n", table_size, total_elements);
    printf("Threads: %d, operations per thread: %d\n\n", NUM_THREADS, NUM_OPERATIONS);

    // Массив идентификаторов потоков: 4 писателя и 4 читателя (всего 8 потоков)
    HANDLE threads[NUM_THREADS * 2];

    // Массив данных для каждого потока
    thread_data_t data[NUM_THREADS * 2];

    // ===== 2. ТЕСТ ЗАПИСИ ========

    printf("1. Writing %zu elements from %d threads...\n", total_elements, NUM_THREADS);

    /**
     * Запускаем 4 потока-писателя.
     * Все они работают параллельно с одной таблицей.
     * Благодаря Lock-Free механизму (CAS) они не блокируют друг друга.
     */
    for (int i = 0; i < NUM_THREADS; i++) {
        data[i].map = map;
        data[i].thread_id = i;
        threads[i] = CreateThread(NULL, 0, writer_thread, &data[i], 0, NULL);
    }

    // Ждем, пока все 4 потока закончат работу
    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);

    // освобождаем ресурсы
    for (int i = 0; i < NUM_THREADS; i++) {
        CloseHandle(threads[i]);
    }

    // Проверяем, сколько элементов реально в таблице
    size_t actual_size = hash_map_size(map);
    printf("   Expected: %zu, Actual: %zu\n", total_elements, actual_size);

    if (actual_size == total_elements) {
        printf("   [PASS] All entries inserted successfully\n");
    }
    else {
        printf("   [FAIL] Expected %zu, got %zu\n", total_elements, actual_size);
    }

    // ===== 3. ТЕСТ ЧТЕНИЯ ======

    printf("\n2. Reading from %d threads...\n", NUM_THREADS);

    /**
     * Запускаем 4 потока-читателя.
     * Каждый ищет свои ключи (которые вставил соответствующий писатель).
     * Они тоже работают параллельно
     */
    for (int i = 0; i < NUM_THREADS; i++) {
        data[NUM_THREADS + i].map = map;
        data[NUM_THREADS + i].thread_id = i;
        threads[i] = CreateThread(NULL, 0, reader_thread, &data[NUM_THREADS + i], 0, NULL);
    }

    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);

    // Собираем результаты от всех читателей
    int total_found = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        DWORD result;
        GetExitCodeThread(threads[i], &result);
        total_found += (int)result;   // Суммируем найденные ключи
        CloseHandle(threads[i]);
    }

    printf("   Found: %d out of %zu\n", total_found, total_elements);

    if ((size_t)total_found == total_elements) {
        printf("   [PASS] All entries found by readers\n");
    }
    else {
        printf("   [FAIL] Expected %zu, got %d\n", total_elements, total_found);
    }

    // ===== 4. ТЕСТ УДАЛЕНИЯ =======

    printf("\n3. Removing half of elements...\n");

    /**
     * Каждый поток удаляет первые 500 своих ключей.
     * Всего удаляется 4 × 500 = 2000 элементов.
     */
    for (int i = 0; i < NUM_THREADS; i++) {
        char key[32];
        for (int j = 0; j < NUM_OPERATIONS / 2; j++) {  // 0..499
            sprintf(key, "key_%d_%d", i, j);
            hash_map_remove(map, key);
        }
    }

    // Проверяем размер после удаления
    size_t size_after = hash_map_size(map);
    printf("   Size after removal: %zu\n", size_after);

    // Ожидаем: 4000 - 2000 = 2000
    if (size_after == total_elements / 2) {
        printf("   [PASS] Half of entries removed\n");
    }
    else {
        printf("   [FAIL] Expected %zu, got %zu\n", total_elements / 2, size_after);
    }

    // ===== 5. ОЧИСТКА =====

    hash_map_free(map);
}


int main(void) {


    printf("============================================\n");
    printf("     LOCK-FREE HASH MAP TEST\n");
    printf("============================================\n");

    test_multithreaded();

    printf("\n============================================\n");
    printf("              TEST COMPLETE\n");
    printf("============================================\n");

    return 0;
}