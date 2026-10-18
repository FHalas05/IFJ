//
// Created by Filip Halaš on 18/10/2026.
//

#ifndef IFJ_SYMTABLE_H
#define IFJ_SYMTABLE_H

typedef char *TKey;

typedef struct {
    char *data; //todo
} TData;

typedef struct tavlnode {
    TKey key;
    TData data;
    int height;
    struct tavlnode *left;
    struct tavlnode *right;
} TAVLNode;

typedef struct {
    TAVLNode *root;
} SymTable;

int max(int num1, int num2);
int AVL_node_height(TAVLNode *node);
int get_balance(TAVLNode *node);
void update_node_height(TAVLNode *node);
TAVLNode* AVL_min(TAVLNode *root);
TAVLNode *right_rotate(TAVLNode *root);
TAVLNode *left_rotate(TAVLNode *root);

TAVLNode* create_AVL_node(TKey k, TData d);
TAVLNode* search_AVL_tree(TAVLNode *root, TKey k);
TAVLNode* insert(TAVLNode *root, TKey k, TData d);
TAVLNode* AVL_delete(TAVLNode *root, TKey k);
void destroy_AVL_tree(TAVLNode *root);
void init_SymTable(SymTable *symtable);
void delete_SymTable(SymTable *symtable);


#endif // IFJ_SYMTABLE_H
