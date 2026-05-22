#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avl.h"

#define PASS "\033[32mPASS\033[0m"
#define FAIL "\033[31mFAIL\033[0m"

static int passed = 0;
static int failed = 0;

static void check(const char* name, int ok) {
    if (ok) { printf("[%s] %s\n", PASS, name); passed++; }
    else     { printf("[%s] %s\n", FAIL, name); failed++; }
}

/* создание дерева */
static void test_create(void) {
    AVLTree* t = createAVLTree();
    check("create: not null",  t != NULL);
    check("create: root null", t->root == NULL);
    check("create: size 0",    t->size == 0);
    freeAVLTree(t);
}

/* вставка одного слова */
static void test_insert_one(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "python", 1, "Learn Python");
    check("insert one: size 1", t->size == 1);

    Vector* p = avlSearch(t, "python");
    check("insert one: found",  p != NULL);
    check("insert one: postings size 1", p != NULL && p->size == 1);

    if (p) {
        PostingEntry* e = getVectorItem(p, 0);
        check("insert one: doc_id", e->doc_id == 1);
        check("insert one: title",  strcmp(e->title, "Learn Python") == 0);
    }
    freeAVLTree(t);
}

/* одно слово в нескольких документах */
static void test_insert_duplicate_key(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "python", 1, "Learn Python");
    avlInsert(t, "python", 2, "Python tips");
    check("dup key: size still 1", t->size == 1);

    Vector* p = avlSearch(t, "python");
    check("dup key: postings size 2", p != NULL && p->size == 2);
    freeAVLTree(t);
}

/* поиск несуществующего слова */
static void test_search_missing(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "python", 1, "Learn Python");
    Vector* p = avlSearch(t, "java");
    check("search missing: null", p == NULL);
    freeAVLTree(t);
}

/* балансировка LL */
static void test_balance_ll(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "cat", 1, "doc1");
    avlInsert(t, "bat", 2, "doc2");
    avlInsert(t, "ant", 3, "doc3");
    check("balance LL: size 3", t->size == 3);
    check("balance LL: find ant", avlSearch(t, "ant") != NULL);
    check("balance LL: find bat", avlSearch(t, "bat") != NULL);
    check("balance LL: find cat", avlSearch(t, "cat") != NULL);
    freeAVLTree(t);
}

/* балансировка RR */
static void test_balance_rr(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "ant", 1, "doc1");
    avlInsert(t, "bat", 2, "doc2");
    avlInsert(t, "cat", 3, "doc3");
    check("balance RR: size 3", t->size == 3);
    check("balance RR: find ant", avlSearch(t, "ant") != NULL);
    check("balance RR: find bat", avlSearch(t, "bat") != NULL);
    check("balance RR: find cat", avlSearch(t, "cat") != NULL);
    freeAVLTree(t);
}

/* балансировка LR */
static void test_balance_lr(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "cat", 1, "doc1");
    avlInsert(t, "ant", 2, "doc2");
    avlInsert(t, "bat", 3, "doc3");
    check("balance LR: size 3", t->size == 3);
    check("balance LR: find all", avlSearch(t, "bat") != NULL);
    freeAVLTree(t);
}

/* балансировка RL */
static void test_balance_rl(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "ant", 1, "doc1");
    avlInsert(t, "cat", 2, "doc2");
    avlInsert(t, "bat", 3, "doc3");
    check("balance RL: size 3", t->size == 3);
    check("balance RL: find all", avlSearch(t, "bat") != NULL);
    freeAVLTree(t);
}

static void count_visit(const char* key, Vector* postings, void* ctx) {
    (void)key; (void)postings;
    (*(int*)ctx)++;
}

/* обход traverse */
static void test_traverse(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "dog", 1, "doc1");
    avlInsert(t, "cat", 2, "doc2");
    avlInsert(t, "elk", 3, "doc3");
    avlInsert(t, "ant", 4, "doc4");

    int count = 0;
    avlTraverse(t, count_visit, &count);
    check("traverse: visited all 4 nodes", count == 4);
    freeAVLTree(t);
}

int main(void) {
    printf("AVL Tree Tests\n");
    test_create();
    test_insert_one();
    test_insert_duplicate_key();
    test_search_missing();
    test_balance_ll();
    test_balance_rr();
    test_balance_lr();
    test_balance_rl();
    test_traverse();
    printf("======================\n");
    printf("passed: %d  failed: %d\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
