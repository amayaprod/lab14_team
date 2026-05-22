#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "btree.h"


void test_basic_insert_search() {
    printf("Test 1: Basic Insert and Search - ");
    BTree* tree = createBTree();
    
    btreeInsert(tree, "apple", 1, "Doc Apple");
    btreeInsert(tree, "banana", 2, "Doc Banana");
    
    Vector* res = btreeSearch(tree, "banana");
    assert(res != NULL && res->size == 1);
    PostingEntry* entry = (PostingEntry*)getVectorItem(res, 0);
    assert(entry->doc_id == 2);
    assert(strcmp(entry->title, "Doc Banana") == 0);
    
    assert(btreeSearch(tree, "cherry") == NULL);
    
    freeBTree(tree);
    printf("PASSED\n");
}

void test_duplicate_insert() {
    printf("Test 2: Duplicate Keys - ");
    BTree* tree = createBTree();
    
    btreeInsert(tree, "python", 10, "Intro to Python");
    btreeInsert(tree, "python", 15, "Python lists");
    btreeInsert(tree, "python", 20, "Advanced Python");
    
    assert(tree->size == 1);
    
    Vector* res = btreeSearch(tree, "python");
    assert(res != NULL && res->size == 3);
    
    PostingEntry* e1 = (PostingEntry*)getVectorItem(res, 0);
    PostingEntry* e3 = (PostingEntry*)getVectorItem(res, 2);
    assert(e1->doc_id == 10);
    assert(e3->doc_id == 20);
    
    freeBTree(tree);
    printf("PASSED\n");
}

void test_node_split() {
    printf("Test 3: Node Splitting - T=3, MAX_KEYS=5 - ");
    BTree* tree = createBTree();
    
    // 6 ключей, чтобы корень точно лопнул
    btreeInsert(tree, "A", 1, "Title A");
    btreeInsert(tree, "B", 2, "Title B");
    btreeInsert(tree, "C", 3, "Title C");
    btreeInsert(tree, "D", 4, "Title D");
    btreeInsert(tree, "E", 5, "Title E");
    // следующая вставка вызовет сплит
    btreeInsert(tree, "F", 6, "Title F");
    
    assert(tree->size == 6);
    
    assert(btreeSearch(tree, "A") != NULL);
    assert(btreeSearch(tree, "F") != NULL);
    assert(btreeSearch(tree, "C") != NULL);
    
    freeBTree(tree);
    printf("PASSED\n");
}

void test_stress() {
    printf("Test 4: Stress Test - 1000 insertions - ");
    BTree* tree = createBTree();
    
    char buffer[32];
    for (int i = 0; i < 1000; i++) {
        snprintf(buffer, sizeof(buffer), "word_%04d", i);
        btreeInsert(tree, buffer, i, "Stress Title");
    }
    
    assert(tree->size == 1000);
    
    Vector* res = btreeSearch(tree, "word_0500");
    assert(res != NULL);
    PostingEntry* e = (PostingEntry*)getVectorItem(res, 0);
    assert(e->doc_id == 500);
    
    freeBTree(tree);
    printf("PASSED\n");
}

static int traverse_count = 0;
static char last_visited_key[256] = "";

void traverse_checker(const char* key, Vector* postings, void* ctx) {
    (void)postings;
    (void)ctx;
    
    if (traverse_count > 0) {
        assert(strcmp(last_visited_key, key) < 0);
    }
    strcpy(last_visited_key, key);
    traverse_count++;
}

void test_traverse_order() {
    printf("Test 5: Traverse Alphabetical Order - ");
    BTree* tree = createBTree();
    traverse_count = 0;
    
    btreeInsert(tree, "Zebra", 1, "T");
    btreeInsert(tree, "Apple", 2, "T");
    btreeInsert(tree, "Mango", 3, "T");
    btreeInsert(tree, "Banana", 4, "T");
    btreeInsert(tree, "Kiwi", 5, "T");
    
    btreeTraverse(tree, traverse_checker, NULL);
    
    assert(traverse_count == 5);
    
    freeBTree(tree);
    printf("PASSED\n");
}

int main() {
    printf("Starting B-Tree Tests\n");
    test_basic_insert_search();
    test_duplicate_insert();
    test_node_split();
    test_stress();
    test_traverse_order();
    printf("All tests passed successfully!\n");
    return 0;
}