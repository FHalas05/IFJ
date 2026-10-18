//
// Created by Filip Halaš on 18/10/2026.
//

#include "symtable.h"
#include "library.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void PrintAVLTree(const TAVLNode *root, int depth) {
    if (root == NULL) {
        return;
    }

    PrintAVLTree(root->right, depth + 1);

    printf("%*s%s(%d)\n", depth * 4, "", root->key, root->height);

    PrintAVLTree(root->left, depth + 1);
}

void PrintSymTable(const SymTable *table) {
    PrintAVLTree(table->root, 0);
}

int max(int num1, int num2) {
    if(num1 > num2) {
        return num1;
    }

    return num2;
}

int AVL_node_height(TAVLNode *node) {
    if(node == NULL) {
        return 0;
    }
    
    return node->height;
}

int get_balance(TAVLNode *node) {
    if(node == NULL) {
        return 0;
    }
    return AVL_node_height(node->right) - AVL_node_height(node->left);
}

void update_node_height(TAVLNode *node) {
    if(node == NULL) {
        return;
    }
    node->height = max(AVL_node_height(node->right), AVL_node_height(node->left)) + 1;
}

TAVLNode* search_AVL_tree(TAVLNode *root, TKey k) {
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

TAVLNode* create_AVL_node(TKey k, TData d) {
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

TAVLNode* insert(TAVLNode *root, TKey k, TData d) {
    if(root == NULL) {
        return create_AVL_node(k, d);
    }

    int cmp = strcmp(k, root->key);
    if(cmp == 0) {
        root->data = d;
    } else {
        if(cmp > 0) {
            root->right = insert(root->right, k, d);
        } else {
            root->left =  insert(root->left, k, d);
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

TAVLNode* AVL_min(TAVLNode *root) {
    if(root->left == NULL) {
        return root;
    }
    return AVL_min(root->left);
}

TAVLNode* AVL_delete(TAVLNode *root, TKey k) {
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

TAVLNode *right_rotate(TAVLNode *root) {
    TAVLNode *newRoot = root->left;
    root->left = newRoot->right;
    newRoot->right = root;
    update_node_height(root);
    update_node_height(newRoot);

    return newRoot;
}

TAVLNode *left_rotate(TAVLNode *root) {
    TAVLNode *newRoot = root->right;
    root->right = newRoot->left;
    newRoot->left = root;
    update_node_height(root);
    update_node_height(newRoot);

    return newRoot;
}

void destroy_AVL_tree(TAVLNode *root) {
    if(root == NULL) {
        return;
    }
    destroy_AVL_tree(root->right);
    destroy_AVL_tree(root->left);
    free(root);
}

void init_SymTable(SymTable *symtable) {
    symtable->root = NULL;
}

void delete_SymTable(SymTable *symtable) {
    destroy_AVL_tree(symtable->root);
    symtable->root = NULL;
}

int main() {
    TData data;
    data.data = "test";
    SymTable table;
    init_SymTable(&table);

    table.root = insert(table.root, "a", data);
    table.root = insert(table.root, "b", data);
    table.root = insert(table.root, "c", data);

    PrintSymTable(&table);

    delete_SymTable(&table);

    return 0;
}