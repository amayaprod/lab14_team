#include "rbtree.h"

static char* StrDup(const char* s)
{
    if (s == NULL)
    {
        return NULL;
    }

    size_t len = strlen(s) + 1;

    char* new_s = malloc(len);
    if (new_s == NULL)
    {
        return NULL;
    }

    memcpy(new_s, s, len);

    return new_s;
}

RBTree* createRBTree(void)
{
    RBTree* tree = malloc(sizeof(RBTree));
    if (tree == NULL)
    {
        return NULL;
    }

    tree->nil = malloc(sizeof(RBNode));
    if (tree->nil == NULL)
    {
        free(tree);
        return NULL;
    }

    tree->nil->color = RB_BLACK;
    tree->nil->left = tree->nil;
    tree->nil->right = tree->nil;
    tree->nil->parent = tree->nil;
    tree->nil->key = NULL;
    tree->nil->postings = NULL;

    tree->root = tree->nil;
    tree->size = 0;

    return tree;
}

static void freeRBNode(RBNode* node, RBNode* nil)
{
    if (node == nil)
    {
        return;
    }

    freeRBNode(node->left, nil);
    freeRBNode(node->right, nil);

    free(node->key);

    if (node->postings != NULL)
    {
        vectorFree(node->postings);
    }

    free(node);
}

void freeRBTree(RBTree* tree)
{
    if (tree == NULL)
    {
        return;
    }

    freeRBNode(tree->root, tree->nil);

    free(tree->nil);
    free(tree);
}


static int rbCase(RBTree* tree, RBNode* z)
{
    RBNode* p = z->parent;
    RBNode* g = p->parent;

    if (p == tree->nil || g == tree->nil)
        {
            return 0;
        }

    RBNode* u;
    if (g->left == p)
    {
        u = g->right;
    }
    else
    {
        u = g->left;
    }

    if (p->color == RB_RED && u->color == RB_RED)
        {
            return 1;
        }

    if (p->color == RB_RED && u->color == RB_BLACK)
    {
        if ((g->left == p && p->left == z) || (g->right == p && p->right == z))
            {
                return 3; 
            }
            else
            {
                return 2;
            }
    }

    return 0;
}

static void leftRotate(RBTree* tree, RBNode* x)
{
    RBNode* y = x->right;

    x->right = y->left;

    if (y->left != tree->nil)
        y->left->parent = x;

    y->parent = x->parent;

    if (x->parent == tree->nil)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    y->left = x;
    x->parent = y;
}

static void rightRotate(RBTree* tree, RBNode* x)
{
    RBNode* y = x->left;

    x->left = y->right;

    if (y->right != tree->nil)
        y->right->parent = x;

    y->parent = x->parent;

    if (x->parent == tree->nil)
        tree->root = y;
    else if (x == x->parent->right)
        x->parent->right = y;
    else
        x->parent->left = y;

    y->right = x;
    x->parent = y;
}

static void rbBalance(RBTree* tree, RBNode* z)
{
    while (z->parent->color == RB_RED)
    {
        RBNode* p = z->parent;
        RBNode* g = p->parent;

        if (g == tree->nil)
            {
                break;
            }

        RBNode* u;
        if (g->left == p)
        {
            u = g->right;
        }
        else
        {
            u = g->left;
        }

        int type = rbCase(tree, z);

        switch (type)
        {
            case 1:
                p->color = RB_BLACK;
                u->color = RB_BLACK;
                g->color = RB_RED;
                z = g;
                break;

            case 2:
                if (g->left == p)
                    {
                        leftRotate(tree, p);
                    }
                else
                    {
                        rightRotate(tree, p);
                    }
                z = p;
                break;

            case 3:
                if (g->left == p)
                {
                    rightRotate(tree, g);
                    p->color = RB_BLACK;
                    g->color = RB_RED;
                }
                else
                {
                    leftRotate(tree, g);
                    p->color = RB_BLACK;
                    g->color = RB_RED;
                }
                z=p;
                break;

            default:
                break;
        }
    }

    tree->root->color = RB_BLACK;
}

void rbInsert(RBTree* tree, const char* key, int doc_id, const char* title)
{
    if (tree == NULL || key == NULL)
        {
            return;
        }

    RBNode* node_to = tree->root;
    RBNode* parent = tree->nil;

    while (node_to != tree->nil)
    {
        int cmp = strcmp(key, node_to->key);
        parent = node_to;

        if (cmp == 0)
        {
            if (node_to->postings == NULL)
            {
                node_to->postings = createPostingList();
            }

            appendPosting(node_to->postings, doc_id, title);
            return;
        }
        else if (cmp < 0)
        {
            node_to = node_to->left;
        }
        else
        {
            node_to = node_to->right;
        }
    }

    RBNode* node = malloc(sizeof(RBNode));
    if (node == NULL)
        {
            return;
        }

    node->key = StrDup(key);
    if (node->key == NULL)
    {
        free(node);
        return;
    }

    node->postings = createPostingList();
    if (node->postings == NULL)
    {
        free(node->key);
        free(node);
        return;
    }

    appendPosting(node->postings, doc_id, title);

    node->color = RB_RED;
    node->left = tree->nil;
    node->right = tree->nil;
    node->parent = tree->nil;

    node->parent = parent;

    if (parent == tree->nil)
    {
        tree->root = node;
    }
    else if (strcmp(key, parent->key) < 0)
    {
        parent->left = node;
    }
    else
    {
        parent->right = node;
    }

    tree->size++;

    rbBalance(tree, node);
}

Vector* rbSearch(const RBTree* tree, const char* key)
{
    RBNode* cur = tree->root;

    if (tree == NULL || key == NULL)
    {
        return NULL;
    }

    while (cur != tree->nil)
    {
        int cmp = strcmp(key, cur->key);

        if (cmp == 0)
        {
            return cur->postings;
        }
        else if (cmp < 0)
        {
            cur = cur->left;
        }
        else
        {
            cur = cur->right;
        }
    }

    return NULL;
}

static void inorder(
    RBTree* tree,
    RBNode* node,
    void (*visit)(const char*, Vector*, void*),
    void* ctx
)
{
    if (node == tree->nil)
    {
        return;
    }

    inorder(tree, node->left, visit, ctx);
    visit(node->key, node->postings, ctx);
    inorder(tree, node->right, visit, ctx);
}

void rbTraverse(
    const RBTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
)
{
    inorder((RBTree*)tree, tree->root, visit, ctx);
}