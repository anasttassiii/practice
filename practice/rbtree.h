#ifndef RBTREE_H
#define RBTREE_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { RED, BLACK } color_t;

typedef struct rb_node_t {
    int key;
    int value;
    color_t color;
    struct rb_node_t* left;
    struct rb_node_t* right;
    struct rb_node_t* parent;
} rb_node_t;

typedef struct {
    rb_node_t* root;
    rb_node_t* nil;
} rb_tree_t;

// ===== Создание/удаление =====
rb_tree_t* rb_create(void);
void rb_free(rb_tree_t* tree);
void rb_free_node(rb_node_t* node);

// ===== Основные операции =====
void rb_insert(rb_tree_t* tree, int key, int value);
bool rb_search(rb_tree_t* tree, int key, int* value);
void rb_delete(rb_tree_t* tree, int key);

// ===== Обходы (принимают tree И node) =====
void rb_inorder(rb_tree_t* tree, rb_node_t* node);
void rb_preorder(rb_tree_t* tree, rb_node_t* node);
void rb_postorder(rb_tree_t* tree, rb_node_t* node);

// ===== Вспомогательные =====
bool rb_is_empty(rb_tree_t* tree);
size_t rb_size(rb_node_t* node);
void rb_print(rb_tree_t* tree);

#endif /* RBTREE_H */