#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btree.h"


// новый пустой узел
static BTreeNode* createNode(int is_leaf) {
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    if (!node) {
        fprintf(stderr, "BTree: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->n = 0;
    node->is_leaf = is_leaf;
    
    for (int i = 0; i < BTREE_MAX_KEYS; i++) {
        node->keys[i] = NULL;
        node->postings[i] = NULL;
    }
    for (int i = 0; i < BTREE_MAX_CH; i++) {
        node->children[i] = NULL;
    }
    return node;
}

// рекурсивно удаляет узел и всех его детей
static void freeNode(BTreeNode* node) {
    if (!node) return;

    for (int i = 0; i < node->n; i++) {
        free(node->keys[i]);
        vectorFree(node->postings[i]);
    }

    if (!node->is_leaf) {
        for (int i = 0; i <= node->n; i++) {
            freeNode(node->children[i]);
        }
    }
    free(node);
}

// создание и удаление дерева

BTree* createBTree(void) {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    if (!tree) {
        fprintf(stderr, "BTree: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    tree->root = createNode(1); // корень сначала  лист
    tree->size = 0;
    return tree;
}

void freeBTree(BTree* tree) {
    if (!tree) return;
    freeNode(tree->root);
    free(tree);
}

// поиск

static Vector* searchNode(BTreeNode* node, const char* key) {
    int i = 0;
    // ищем первый ключ, keys[i] >= key
    while (i < node->n && strcmp(key, node->keys[i]) > 0) {
        i++;
    }

    if (i < node->n && strcmp(key, node->keys[i]) == 0) {
        return node->postings[i];
    }

    if (node->is_leaf) {
        return NULL;
    }

    return searchNode(node->children[i], key);
}

Vector* btreeSearch(const BTree* tree, const char* key) {
    if (!tree || !tree->root) return NULL;
    return searchNode(tree->root, key);
}

// обход дерева

static void traverseNode(BTreeNode* node, 
                         void (*visit)(const char* key, Vector* postings, void* ctx), 
                         void* ctx) {
    if (!node) return;

    int i;
    for (i = 0; i < node->n; i++) {
        // сначала левый чилд
        if (!node->is_leaf) {
            traverseNode(node->children[i], visit, ctx);
        }
        visit(node->keys[i], node->postings[i], ctx);
    }
    // правый
    if (!node->is_leaf) {
        traverseNode(node->children[i], visit, ctx);
    }
}

void btreeTraverse(const BTree* tree, 
                   void (*visit)(const char* key, Vector* postings, void* ctx), 
                   void* ctx) {
    if (!tree || !tree->root) return;
    traverseNode(tree->root, visit, ctx);
}

// вставка и балансировка

// разбивание переполненного ребенка, x - родитель, i - индекс переполненного ребенка

static void splitChild(BTreeNode* x, int i) {
    BTreeNode* y = x->children[i];
    BTreeNode* z = createNode(y->is_leaf);
    
    z->n = BTREE_T - 1;
    
    // перенос последних 2 ключей из y в z
    for (int j = 0; j < BTREE_T - 1; j++) {
        z->keys[j] = y->keys[j + BTREE_T];
        z->postings[j] = y->postings[j + BTREE_T];
        
        y->keys[j + BTREE_T] = NULL;
        y->postings[j + BTREE_T] = NULL;
    }
    
    // если узлы не листья - переносим и последних 3х детей
    if (!y->is_leaf) {
        for (int j = 0; j < BTREE_T; j++) {
            z->children[j] = y->children[j + BTREE_T];
            y->children[j + BTREE_T] = NULL;
        }
    }
    y->n = BTREE_T - 1;
    
    // сдвиг детей в родителе x, чтобы освободить место для z
    for (int j = x->n; j >= i + 1; j--) {
        x->children[j + 1] = x->children[j];
    }
    x->children[i + 1] = z;

    // сдвиг ключей в родителе x, чтобы поднять средний ключ из y
    for (int j = x->n - 1; j >= i; j--) {
        x->keys[j + 1] = x->keys[j];
        x->postings[j + 1] = x->postings[j];
    }
    
    // поднимаем средний ключ в x
    x->keys[i] = y->keys[BTREE_T - 1];
    x->postings[i] = y->postings[BTREE_T - 1];
    y->keys[BTREE_T - 1] = NULL;
    y->postings[BTREE_T - 1] = NULL;
    
    x->n += 1;
}

// рекурсивная вставка в непереполненный узел
static void insertNonFull(BTree* tree, BTreeNode* x, const char* key, int doc_id, const char* title) {
    int i = 0;
    
    while (i < x->n && strcmp(key, x->keys[i]) > 0) {
        i++;
    }
    
    // граничный случай 1 - ключ уже существует
    if (i < x->n && strcmp(key, x->keys[i]) == 0) {
        appendPosting(x->postings[i], doc_id, title);
        return;
    }
    
    if (x->is_leaf) {
        // если это лист, сдвигаем всё вправо и вставляем новый ключ
        for (int j = x->n - 1; j >= i; j--) {
            x->keys[j + 1] = x->keys[j];
            x->postings[j + 1] = x->postings[j];
        }
        x->keys[i] = strdup(key);
        x->postings[i] = createPostingList();
        appendPosting(x->postings[i], doc_id, title);
        
        x->n += 1;
        tree->size += 1;
    } else {
        if (x->children[i]->n == BTREE_MAX_KEYS) {
            // граничный случай 2 - ребенок переполнен
            splitChild(x, i);
            
            int cmp = strcmp(key, x->keys[i]);
            if (cmp == 0) {
                appendPosting(x->postings[i], doc_id, title);
                return;
            } else if (cmp > 0) {
                i++;
            }
        }
        insertNonFull(tree, x->children[i], key, doc_id, title);
    }
}

void btreeInsert(BTree* tree, const char* key, int doc_id, const char* title) {
    if (!tree || !tree->root) return;
    
    BTreeNode* root = tree->root;
    // граничный случай 3 - корень переполнен
    if (root->n == BTREE_MAX_KEYS) {
        BTreeNode* new_root = createNode(0);
        tree->root = new_root;
        new_root->children[0] = root;
        
        splitChild(new_root, 0);
        insertNonFull(tree, new_root, key, doc_id, title);
    } else {
        insertNonFull(tree, root, key, doc_id, title);
    }
}
