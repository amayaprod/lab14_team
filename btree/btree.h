#ifndef BTREE_H
#define BTREE_H

#include "../posting.h"

#define BTREE_T        3 // min степень, узел хранит от t-1 до 2t-1 ключей
#define BTREE_MAX_KEYS (2 * BTREE_T - 1) // max 5 ключей
#define BTREE_MAX_CH   (2 * BTREE_T) // max 6 потомков

typedef struct BTreeNode {
    char*             keys[BTREE_MAX_KEYS];      // массив ключей
    Vector*           postings[BTREE_MAX_KEYS];  // массив posting listов
    struct BTreeNode* children[BTREE_MAX_CH];    // массив указателей на детей
    int               n;                         // текущее количество ключей в узле
    int               is_leaf;                   // 1, если узел - лист, иначе 0
} BTreeNode;

typedef struct {
    BTreeNode* root;
    int        size; // количество уникальных слов в дереве
} BTree;

// создание/удаление дерева
BTree*  createBTree(void);
void    freeBTree(BTree* tree);

// основа
void    btreeInsert(BTree* tree, const char* key, int doc_id, const char* title);
Vector* btreeSearch(const BTree* tree, const char* key);

// обход дерева - для fuzzy search
void    btreeTraverse(const BTree* tree,
                      void (*visit)(const char* key, Vector* postings, void* ctx),
                      void* ctx);

#endif