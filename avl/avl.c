#include <stdlib.h>
#include <string.h>
#include "avl.h"

static int height(AVLNode* n){
    return n ? n->height : 0;
}

static int max(int a, int b) {
    return a > b ? a : b;
}

static int balanceFactor(AVLNode* n){
    return n ? height(n->left) - height(n->right) : 0;
}

static AVLNode* newNode(const char* str, int doc_id, const char* title) {
    AVLNode* node = malloc(sizeof(AVLNode));
    node->height = 1;
    node->key = strdup(str);
    node->postings = createPostingList();
    appendPosting(node->postings, doc_id, title);
    node->left = NULL;
    node->right = NULL;
    return node;
}

static AVLNode* rotateRight(AVLNode* n) {
    AVLNode* x = n->left;
    AVLNode* y = x->right;
    x->right = n;
    n->left = y;
    n->height = 1 + max(height(n->left), height(n->right));
    x->height = 1 + max(height(x->left), height(x->right));
    return x;
}

static AVLNode* rotateLeft(AVLNode* n) {
    AVLNode* x = n->right;
    AVLNode* y = x->left;
    x->left = n;
    n->right = y;
    n->height = 1 + max(height(n->left), height(n->right));
    x->height = 1 + max(height(x->left), height(x->right));
    return x;
}

static AVLNode* insertNode(AVLNode* node, const char* str, int doc_id, const char* title) {
    if (node == NULL) {
        return newNode(str, doc_id, title);
    }

    int cmp = strcmp(str, node->key);
    if (cmp < 0) {
        node->left = insertNode(node->left, str, doc_id, title);
    } else if (cmp > 0) {
        node->right = insertNode(node->right, str, doc_id, title);
    } else {
        appendPosting(node->postings, doc_id, title);
        return node;
    }
    node->height = 1 + max(height(node->left), height(node->right));

    int bf = balanceFactor(node);
    if (bf > 1 && strcmp(str, node->left->key) < 0) {
        return rotateRight(node);
    } else if (bf < -1 && strcmp(str, node->right->key) > 0) {
        return rotateLeft(node);
    } else if (bf > 1 && strcmp(str, node->left->key) > 0) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    } else if (bf < -1 && strcmp(str, node->right->key) < 0) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }
    return node;
}

AVLTree* createAVLTree() {
    AVLTree* tree = malloc(sizeof(AVLTree));
    tree->root = NULL;
    tree->size = 0;
    return tree;
}

void avlInsert(AVLTree* tree, const char* key, int doc_id, const char* title) {
    if (avlSearch(tree, key) == NULL) {
    tree->root = insertNode(tree->root, key, doc_id, title);
    tree->size++;
    } else {
        tree->root = insertNode(tree->root, key, doc_id, title);
    }
}

Vector* avlSearch(const AVLTree* tree, const char* key) {
    AVLNode* node = tree->root;
    while (node != NULL) {
        int cmp = strcmp(key, node->key);
        if (cmp < 0) {
            node = node->left;
        } else if (cmp > 0) {
            node = node->right;
        } else {
            return node->postings;
        }
    }
    return NULL;
}

static void traverseNode(
     AVLNode* node,
     void (*visit)(const char* key, Vector* postings, void* ctx),
     void* ctx) {
    if (node == NULL) {
        return;
    }
    traverseNode(node->left, visit, ctx);
    visit(node->key, node->postings, ctx);
    traverseNode(node->right, visit, ctx);
}

void avlTraverse(
    const AVLTree* tree,
    void (*visit)(const char* key,Vector* postings, void* ctx),
    void* ctx) {

    traverseNode(tree->root, visit, ctx);
}

static void freeNode(AVLNode* node) {
    if (node == NULL) {
        return;
    }
    freeNode(node->left);
    freeNode(node->right);
    free(node->key);
    vectorFree(node->postings);
    free(node);
}
void freeAVLTree(AVLTree* tree) {
    freeNode(tree->root);
    free(tree);
}