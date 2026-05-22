// Полностью на ваше усмотрение (только переиспользуйте код из предыдущих лабораторных, если он вам подходит)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"
#include "../avl/avl.h"
#include "../rbtree/rbtree.h"
#include "../btree/btree.h"


Index* createIndex(TreeType type) {
    Index* idx = (Index*)malloc(sizeof(Index));
    if (!idx) return NULL;
    idx->type = type;
    switch (type) {
        case TREE_AVL:
            idx->tree = createAVLTree();
            break;
        case TREE_RB:
            idx->tree = createRBTree();
            break;
        case TREE_BTREE:
            idx->tree = createBTree();
            break;
    }
    return idx;
}

void insertTerm(Index* idx, const char* term, int doc_id, const char* title) {
    if (!idx || !idx->tree) return;
    switch (idx->type) {
        case TREE_AVL:
            avlInsert((AVLTree*)idx->tree, term, doc_id, title);
            break;
        case TREE_RB:
            rbInsert((RBTree*)idx->tree, term, doc_id, title);
            break;
        case TREE_BTREE:
            btreeInsert((BTree*)idx->tree, term, doc_id, title);
            break;
    }
}

Vector* lookupTerm(const Index* idx, const char* term) {
    if (!idx || !idx->tree) return NULL;
    switch (idx->type) {
        case TREE_AVL:
            return avlSearch((const AVLTree*)idx->tree, term);
        case TREE_RB:
            return rbSearch((const RBTree*)idx->tree, term);
        case TREE_BTREE:
            return btreeSearch((const BTree*)idx->tree, term);
    }
    return NULL;
}

void indexDocument(Index* idx, int doc_id, const char* title,
                       const char** tokens, int n_tokens) {
    if (!idx) return;
    for (int i = 0; i < n_tokens; i++) {
        int is_duplicate = 0;
        for (int j = 0; j < i; j++) {
            if (strcmp(tokens[i], tokens[j]) == 0) {
                is_duplicate = 1;
                break;
            }
        }
        if (!is_duplicate) {
            insertTerm(idx, tokens[i], doc_id, title);
        }
    }
}

void traverseIndex(const Index* idx,
                   void (*visit)(const char* key, Vector* postings, void* ctx),
                   void* ctx) {
    if (!idx || !idx->tree) return;
    switch (idx->type) {
        case TREE_AVL:
            avlTraverse((const AVLTree*)idx->tree, visit, ctx);
            break;
        case TREE_RB:
            rbTraverse((const RBTree*)idx->tree, visit, ctx);
            break;
        case TREE_BTREE:
            btreeTraverse((const BTree*)idx->tree, visit, ctx);
            break;
    }
}

static void save_callback(const char* key, Vector* postings, void* ctx) {
    FILE* f = (FILE*)ctx;
    if (!f || !postings) return;
    fprintf(f, "@TERM:%s %zu\n", key, postings->size);
    for (size_t i = 0; i < postings->size; i++) {
        PostingEntry* pe = (PostingEntry*)getVectorItem(postings, i);
        if (pe) {
            fprintf(f, "%d:%s\n", pe->doc_id, pe->title);
        }
    }
}

void saveIndex(const Index* idx, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) {
        perror("Ошибка открытия файла для записи индекса");
        return;
    }
    traverseIndex(idx, save_callback, f);
    fclose(f);
}

Index* loadIndex(const char* path, TreeType type) {
    FILE* f = fopen(path, "r");
    if (!f) {
        return NULL;
    }

    Index* idx = createIndex(type);
    if (!idx) {
        fclose(f);
        return NULL;
    }

    char line[1024];
    char current_term[256] = {0};
    int postings_to_read = 0;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (strncmp(line, "@TERM:", 6) == 0) {
            char* term_start = line + 6;
            char* space = strrchr(term_start, ' ');
            if (space) {
                *space = '\0';
                strncpy(current_term, term_start, sizeof(current_term) - 1);
                current_term[sizeof(current_term) - 1] = '\0';
                postings_to_read = atoi(space + 1);
            }
        } else {
            char* colon = strchr(line, ':');
            if (colon && postings_to_read > 0) {
                *colon = '\0';
                int doc_id = atoi(line);
                char* title = colon + 1;
                insertTerm(idx, current_term, doc_id, title);
                postings_to_read--;
            }
        }
    }

    fclose(f);
    return idx;
}

void freeIndex(Index* idx) {
    if (!idx) return;
    if (idx->tree) {
        switch (idx->type) {
            case TREE_AVL:
                freeAVLTree((AVLTree*)idx->tree);
                break;
            case TREE_RB:
                freeRBTree((RBTree*)idx->tree);
                break;
            case TREE_BTREE:
                freeBTree((BTree*)idx->tree);
                break;
        }
    }
    free(idx);
}

TreeType parseType(const char* type_str) {
    if (strcmp(type_str, "avl") == 0)   return TREE_AVL;
    if (strcmp(type_str, "rb") == 0)    return TREE_RB;
    if (strcmp(type_str, "btree") == 0) return TREE_BTREE;
    return TREE_AVL;
}

const char* typeName(TreeType type) {
    switch (type) {
        case TREE_AVL:   return "avl";
        case TREE_RB:    return "rb";
        case TREE_BTREE: return "btree";
    }
    return "unknown";
}