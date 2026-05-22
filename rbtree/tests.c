#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rbtree.h"
#include "../vector/comparators.h"

void log_result(const char* task, const char* message) {
    printf("[%s] %s\n", task, message);
}

int createRBTree_tests()
{
    log_result("createRBTree", "start testing");

    RBTree* tree = createRBTree();
    assert(tree != NULL);
    assert(tree->root == tree->nil);
    assert(tree->size == 0);

    assert(tree->nil != NULL);
    assert(tree->nil->color == RB_BLACK);
    assert(tree->nil->left == tree->nil);
    assert(tree->nil->right == tree->nil);
    assert(tree->nil->parent == tree->nil);

    log_result("createRBTree", "tests completed\n");
    return 0;
}

int rbInsert_search_tests()
{
    log_result("rbInsert/search", "start testing");

    RBTree* tree = createRBTree();

    rbInsert(tree, "A", 1, "doc");

    Vector* res = rbSearch(tree, "A");
    assert(res != NULL);

    Vector* res2 = rbSearch(tree, "B");
    assert(res2 == NULL);

    assert(tree->size == 1);

    log_result("rbInsert/search", "tests completed\n");
    return 0;
}

int rbDuplicateKey_tests()
{
    log_result("rbInsert duplicate", "start testing");

    RBTree* tree = createRBTree();

    rbInsert(tree, "key", 1, "A");
    rbInsert(tree, "key", 2, "B");
    rbInsert(tree, "key", 3, "C");

    Vector* v = rbSearch(tree, "key");
    assert(v != NULL);

    log_result("rbInsert duplicate", "tests completed\n");
    return 0;
}

static void check_root_black(RBTree* tree)
{
    assert(tree->root->color == RB_BLACK || tree->root == tree->nil);
}

int rbBalance_basic_tests()
{
    log_result("rbBalance", "start testing");

    RBTree* tree = createRBTree();

    rbInsert(tree, "D", 1, "doc");
    rbInsert(tree, "B", 2, "doc");
    rbInsert(tree, "A", 3, "doc");
    rbInsert(tree, "C", 4, "doc");
    rbInsert(tree, "E", 5, "doc");

    check_root_black(tree);

    assert(tree->size == 5);

    assert(rbSearch(tree, "A") != NULL);
    assert(rbSearch(tree, "B") != NULL);
    assert(rbSearch(tree, "C") != NULL);
    assert(rbSearch(tree, "D") != NULL);
    assert(rbSearch(tree, "E") != NULL);

    log_result("rbBalance", "tests completed\n");
    return 0;
}

int rbStress_tests()
{
    log_result("rbStress", "start testing");

    RBTree* tree = createRBTree();

    const int N = 1000;

    char key[32];

    for (int i = 0; i < N; i++)
    {
        sprintf(key, "key_%d", i);
        rbInsert(tree, key, i, "doc");
    }

    assert(tree->size == N);

    for (int i = 0; i < N; i += 50)
    {
        sprintf(key, "key_%d", i);
        Vector* v = rbSearch(tree, key);
        assert(v != NULL);
    }

    log_result("rbStress", "tests completed\n");
    return 0;
}

int rbNull_tests()
{
    log_result("rbNull", "start testing");

    RBTree* tree = createRBTree();

    assert(rbSearch(tree, "N") == NULL);

    rbInsert(tree, "A", 1, "doc");
    rbInsert(tree, "B", 2, "doc");

    assert(rbSearch(tree, NULL) == NULL);

    log_result("rbNull", "tests completed\n");
    return 0;
}

static int counter = 0;

void visit(const char* key, Vector* postings, void* ctx)
{
    (void)key;
    (void)postings;
    (void)ctx;
    counter++;
}

int rbTraverse_tests()
{
    log_result("rbTraverse", "start testing");

    RBTree* tree = createRBTree();

    rbInsert(tree, "C", 1, "doc");
    rbInsert(tree, "A", 2, "doc");
    rbInsert(tree, "B", 3, "doc");

    counter = 0;
    rbTraverse(tree, visit, NULL);

    assert(counter == 3);

    log_result("rbTraverse", "tests completed\n");
    return 0;
}

int main()
{
    createRBTree_tests();
    rbInsert_search_tests();
    rbDuplicateKey_tests();
    rbBalance_basic_tests();
    rbStress_tests();
    rbNull_tests();
    rbTraverse_tests();

    printf("ALL RB-TREE TESTS COMPLETED\n");
    return 0;
}