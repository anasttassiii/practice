#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "rbtree.h"

// ===== Вспомогательные функции =====

static rb_node_t* create_node(rb_tree_t* tree, int key, int value) {
    rb_node_t* node = malloc(sizeof(rb_node_t));
    if (node == NULL) return NULL;

    node->key = key;
    node->value = value;
    node->color = RED;
    node->left = tree->nil;
    node->right = tree->nil;
    node->parent = tree->nil;

    return node;
}

static bool is_red(rb_node_t* node) {
    if (node == NULL) return false;
    return node->color == RED;
}

// ===== Вращения =====

static void rotate_left(rb_tree_t* tree, rb_node_t* x) {
    rb_node_t* y = x->right;
    x->right = y->left;

    if (y->left != tree->nil) {
        y->left->parent = x;
    }

    y->parent = x->parent;

    if (x->parent == tree->nil) {
        tree->root = y;
    }
    else if (x == x->parent->left) {
        x->parent->left = y;
    }
    else {
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}

static void rotate_right(rb_tree_t* tree, rb_node_t* x) {
    rb_node_t* y = x->left;
    x->left = y->right;

    if (y->right != tree->nil) {
        y->right->parent = x;
    }

    y->parent = x->parent;

    if (x->parent == tree->nil) {
        tree->root = y;
    }
    else if (x == x->parent->right) {
        x->parent->right = y;
    }
    else {
        x->parent->left = y;
    }

    y->right = x;
    x->parent = y;
}

// ===== Балансировка после вставки =====

static void rb_insert_fixup(rb_tree_t* tree, rb_node_t* z) {
    while (is_red(z->parent)) {
        if (z->parent == z->parent->parent->left) {
            rb_node_t* y = z->parent->parent->right;

            if (is_red(y)) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            }
            else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rotate_left(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotate_right(tree, z->parent->parent);
            }
        }
        else {
            rb_node_t* y = z->parent->parent->left;

            if (is_red(y)) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            }
            else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rotate_right(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotate_left(tree, z->parent->parent);
            }
        }
    }

    tree->root->color = BLACK;
}

// ===== Вставка =====

void rb_insert(rb_tree_t* tree, int key, int value) {
    if (tree == NULL || tree->nil == NULL) return;

    rb_node_t* z = create_node(tree, key, value);
    if (z == NULL) return;

    rb_node_t* y = tree->nil;
    rb_node_t* x = tree->root;

    while (x != tree->nil) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        }
        else if (z->key > x->key) {
            x = x->right;
        }
        else {
            x->value = value;
            free(z);
            return;
        }
    }

    z->parent = y;

    if (y == tree->nil) {
        tree->root = z;
    }
    else if (z->key < y->key) {
        y->left = z;
    }
    else {
        y->right = z;
    }

    rb_insert_fixup(tree, z);
}

// ===== Балансировка после удаления =====

static void rb_delete_fixup(rb_tree_t* tree, rb_node_t* x) {
    while (x != tree->root && !is_red(x)) {
        if (x == x->parent->left) {
            rb_node_t* w = x->parent->right;

            if (is_red(w)) {
                w->color = BLACK;
                x->parent->color = RED;
                rotate_left(tree, x->parent);
                w = x->parent->right;
            }

            if (!is_red(w->left) && !is_red(w->right)) {
                w->color = RED;
                x = x->parent;
            }
            else {
                if (!is_red(w->right)) {
                    w->left->color = BLACK;
                    w->color = RED;
                    rotate_right(tree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rotate_left(tree, x->parent);
                x = tree->root;
            }
        }
        else {
            rb_node_t* w = x->parent->left;

            if (is_red(w)) {
                w->color = BLACK;
                x->parent->color = RED;
                rotate_right(tree, x->parent);
                w = x->parent->left;
            }

            if (!is_red(w->right) && !is_red(w->left)) {
                w->color = RED;
                x = x->parent;
            }
            else {
                if (!is_red(w->left)) {
                    w->right->color = BLACK;
                    w->color = RED;
                    rotate_left(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rotate_right(tree, x->parent);
                x = tree->root;
            }
        }
    }
    x->color = BLACK;
}

// ===== Поиск минимального узла =====

static rb_node_t* find_min(rb_tree_t* tree, rb_node_t* node) {
    while (node->left != tree->nil) {
        node = node->left;
    }
    return node;
}

// ===== Трансплантация =====

static void transplant(rb_tree_t* tree, rb_node_t* u, rb_node_t* v) {
    if (u->parent == tree->nil) {
        tree->root = v;
    }
    else if (u == u->parent->left) {
        u->parent->left = v;
    }
    else {
        u->parent->right = v;
    }
    v->parent = u->parent;
}

// ===== Удаление =====

void rb_delete(rb_tree_t* tree, int key) {
    if (tree == NULL || tree->root == tree->nil) return;

    rb_node_t* z = tree->root;
    while (z != tree->nil) {
        if (key < z->key) {
            z = z->left;
        }
        else if (key > z->key) {
            z = z->right;
        }
        else {
            break;
        }
    }

    if (z == tree->nil) return;

    rb_node_t* y = z;
    rb_node_t* x;
    color_t y_original_color = y->color;

    if (z->left == tree->nil) {
        x = z->right;
        transplant(tree, z, z->right);
    }
    else if (z->right == tree->nil) {
        x = z->left;
        transplant(tree, z, z->left);
    }
    else {
        y = find_min(tree, z->right);
        y_original_color = y->color;
        x = y->right;

        if (y->parent == z) {
            x->parent = y;
        }
        else {
            transplant(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }

        transplant(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    free(z);

    if (y_original_color == BLACK) {
        rb_delete_fixup(tree, x);
    }
}

// ===== Поиск =====

bool rb_search(rb_tree_t* tree, int key, int* value) {
    if (tree == NULL || tree->root == tree->nil) return false;

    rb_node_t* current = tree->root;

    while (current != tree->nil) {
        if (key < current->key) {
            current = current->left;
        }
        else if (key > current->key) {
            current = current->right;
        }
        else {
            if (value != NULL) {
                *value = current->value;
            }
            return true;
        }
    }

    return false;
}

// ===== Создание дерева =====

rb_tree_t* rb_create(void) {
    rb_tree_t* tree = malloc(sizeof(rb_tree_t));
    if (tree == NULL) return NULL;

    tree->nil = malloc(sizeof(rb_node_t));
    if (tree->nil == NULL) {
        free(tree);
        return NULL;
    }

    tree->nil->color = BLACK;
    tree->nil->left = NULL;
    tree->nil->right = NULL;
    tree->nil->parent = NULL;
    tree->nil->key = 0;
    tree->nil->value = 0;

    tree->root = tree->nil;

    return tree;
}

// ===== Освобождение памяти (ИСПРАВЛЕНО) =====

void rb_free_node(rb_tree_t* tree, rb_node_t* node) {
    if (node == NULL || node == tree->nil) return;  // ✅ Проверка на nil
    rb_free_node(tree, node->left);
    rb_free_node(tree, node->right);
    free(node);
}

void rb_free(rb_tree_t* tree) {
    if (tree == NULL) return;

    if (tree->root != tree->nil) {
        rb_free_node(tree, tree->root);
    }

    free(tree->nil);
    free(tree);
}

// ===== Размер дерева =====

size_t rb_size(rb_node_t* node) {
    if (node == NULL) return 0;
    return 1 + rb_size(node->left) + rb_size(node->right);
}

bool rb_is_empty(rb_tree_t* tree) {
    return tree == NULL || tree->root == tree->nil;
}

// ===== Обходы =====

void rb_inorder(rb_tree_t* tree, rb_node_t* node) {
    if (node == NULL || node == tree->nil) return;
    rb_inorder(tree, node->left);
    printf("%d ", node->key);
    rb_inorder(tree, node->right);
}

void rb_preorder(rb_tree_t* tree, rb_node_t* node) {
    if (node == NULL || node == tree->nil) return;
    printf("%d ", node->key);
    rb_preorder(tree, node->left);
    rb_preorder(tree, node->right);
}

void rb_postorder(rb_tree_t* tree, rb_node_t* node) {
    if (node == NULL || node == tree->nil) return;
    rb_postorder(tree, node->left);
    rb_postorder(tree, node->right);
    printf("%d ", node->key);
}

// ===== Печать дерева =====

void rb_print_node(rb_tree_t* tree, rb_node_t* node, int level, char direction) {
    if (node == NULL || node == tree->nil) return;

    for (int i = 0; i < level; i++) {
        printf("  ");
    }

    printf("%c[%d] (%s)\n", direction, node->key,
        node->color == RED ? "RED" : "BLACK");

    rb_print_node(tree, node->left, level + 1, 'L');
    rb_print_node(tree, node->right, level + 1, 'R');
}

void rb_print(rb_tree_t* tree) {
    if (tree == NULL || tree->root == tree->nil) {
        printf("Empty tree\n");
        return;
    }

    printf("Root: [%d] (%s)\n", tree->root->key,
        tree->root->color == RED ? "RED" : "BLACK");
    rb_print_node(tree, tree->root->left, 1, 'L');
    rb_print_node(tree, tree->root->right, 1, 'R');
}