//
// Created by Filip Halaš on 18/10/2026.
//

#include "symtable.h"
#include "library.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct tavlnode {
    TKey key;
    TData data;
    int height;
    struct tavlnode *left;
    struct tavlnode *right;
} TAVLNode;

struct SymTable{
    TAVLNode *root;
};

static int max(int num1, int num2) {
    if(num1 > num2) {
        return num1;
    }

    return num2;
}

static TAVLNode* AVL_min(TAVLNode *root) {
    if(root->left == NULL) {
        return root;
    }
    return AVL_min(root->left);
}

static int AVL_node_height(TAVLNode *node) {
    if(node == NULL) {
        return 0;
    }
    
    return node->height;
}

static int get_balance(TAVLNode *node) {
    if(node == NULL) {
        return 0;
    }
    return AVL_node_height(node->right) - AVL_node_height(node->left);
}

static void update_node_height(TAVLNode *node) {
    if(node == NULL) {
        return;
    }
    node->height = max(AVL_node_height(node->right), AVL_node_height(node->left)) + 1;
}

static TAVLNode *right_rotate(TAVLNode *root) {
    TAVLNode *newRoot = root->left;
    root->left = newRoot->right;
    newRoot->right = root;
    update_node_height(root);
    update_node_height(newRoot);

    return newRoot;
}

static TAVLNode *left_rotate(TAVLNode *root) {
    TAVLNode *newRoot = root->right;
    root->right = newRoot->left;
    newRoot->left = root;
    update_node_height(root);
    update_node_height(newRoot);

    return newRoot;
}

static TAVLNode* search_AVL_tree(TAVLNode *root, TKey k) {
    if(root == NULL) {
        return NULL;
    }

    int cmp = strcmp(k, root->key);
    if(cmp == 0) {
        return root;
    }
    if(cmp > 0) {
        return search_AVL_tree(root->right, k);
    }
    return search_AVL_tree(root->left, k);
}

static TAVLNode* create_AVL_node(TKey k, TData d) {
    TAVLNode *new = (TAVLNode *)malloc(sizeof(TAVLNode));
    if(new == NULL) {
        throw_fatal_error(99, "Could not allocate memory!\n");
    }
    new->key = k;
    new->data = d;
    new->height = 1;
    new->right = NULL;
    new->left = NULL;

    return new;
}

static TAVLNode* insert_AVL_node(TAVLNode *root, TKey k, TData d) {
    if(root == NULL) {
        return create_AVL_node(k, d);
    }

    int cmp = strcmp(k, root->key);
    if(cmp == 0) {
        root->data = d;
    } else {
        if(cmp > 0) {
            root->right = insert_AVL_node(root->right, k, d);
        } else {
            root->left =  insert_AVL_node(root->left, k, d);
        }
        update_node_height(root);
        int balance = get_balance(root);
        if(balance < -1) {
            int sonBalance = get_balance(root->left);
            if(sonBalance >= 1) {
                root->left = left_rotate(root->left);
            }
            root = right_rotate(root);
        } else if(balance > 1) {
            int sonBalance = get_balance(root->right);
            if(sonBalance <= -1) {
                root->right = right_rotate(root->right);
            }
            root = left_rotate(root);
        }
    }

    return root;
}

static TAVLNode* AVL_delete(TAVLNode *root, TKey k) {
    if(root == NULL) {
        return NULL;
    }

    int cmp = strcmp(k, root->key);
    if(cmp > 0) {
        root->right = AVL_delete(root->right, k);
    } else if(cmp < 0) {
        root->left =  AVL_delete(root->left, k);
    } else {
        if((root->right == NULL) && (root->left == NULL)) {
            free(root);
            return NULL;
        }

        if((root->right == NULL) || (root->left == NULL)) {
            TAVLNode *onlyChild;
            if(root->left == NULL) {
                onlyChild = root->right;
            } else {
                onlyChild = root->left;
            }
            free(root);
            return onlyChild;
        } else {
            TAVLNode *min = AVL_min(root->right);
            root->key = min->key;
            root->data = min->data;
            root->right = AVL_delete(root->right, min->key);
        }
    }

    update_node_height(root);
    int balance = get_balance(root);
    if(balance < -1) {
        int sonBalance = get_balance(root->left);
        if(sonBalance >= 1) {
            root->left = left_rotate(root->left);
        }
        root = right_rotate(root);
    } else if(balance > 1) {
        int sonBalance = get_balance(root->right);
        if(sonBalance <= -1) {
            root->right = right_rotate(root->right);
        }
        root = left_rotate(root);
    }

    return root;
}

static void destroy_AVL_tree(TAVLNode *root) {
    if(root == NULL) {
        return;
    }
    destroy_AVL_tree(root->right);
    destroy_AVL_tree(root->left);
    free(root);
}

SymTable *create_symtable(void) {
    SymTable *table = (SymTable *)malloc(sizeof(SymTable));
    if(table == NULL) {
        throw_fatal_error(99, "Could not allocate memory!\n");
    }
    table->root = NULL;
    return table;
}

void insert(SymTable *symtable, TKey k, TData d) {
    symtable->root = insert_AVL_node(symtable->root, k, d);
}

void symtable_delete(SymTable *symtable, TKey k) {
    symtable->root = AVL_delete(symtable->root, k);
}

TData *search(SymTable *symtable, TKey k) {
    TAVLNode *node = search_AVL_tree(symtable->root, k);
    if(node != NULL) {
        return &node->data;
    }
    return NULL;
}

void destroy_symtable(SymTable *symtable) {
    destroy_AVL_tree(symtable->root);
    free(symtable);
}
