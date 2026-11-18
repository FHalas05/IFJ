/**
 * @file symtable.c
 * @brief AVL-based symbol table and nested scope implementation
 *
 * Implements symbol insertion, search and deletion, AVL tree balancing,
 * and the creation and destruction of symbol tables and scopes.
 *
 * @author Filip Halaš (xhalasf00)
 * @date 18.10.2026
 */

#include "symtable.h"
#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief AVL tree node containing a symbol key and its data
 */
typedef struct tavlnode {
    TKey key;               // Symbol key stored without copying
    TData data;             // Symbol data stored by value
    int height;             // Subtree height; a leaf has height 1
    struct tavlnode *left;  // Child containing smaller keys
    struct tavlnode *right; // Child containing greater keys
} TAVLNode;

/**
 * @brief Internal representation of a symbol table
 */
struct SymTable {
    TAVLNode *root; // Root of the AVL tree, or NULL for an empty table
};

/**
 * @brief Returns the greater of two integers
 *
 * @param num1 First integer
 * @param num2 Second integer
 *
 * @return Greater value, or their common value if equal
 */
static int max(int num1, int num2) {
    if (num1 > num2) {
        return num1;
    }

    return num2;
}

/**
 * @brief Finds the node with the smallest key in a subtree
 *
 * @param root Root of a non-empty subtree
 *
 * @return Pointer to the leftmost node
 */
static TAVLNode *AVL_min(TAVLNode *root) {
    if (root->left == NULL) {
        return root;
    }
    return AVL_min(root->left);
}

/**
 * @brief Returns the height of an AVL subtree
 *
 * @param node Root of the subtree; may be NULL
 *
 * @return Stored node height, or 0 if node is NULL
 */
static int AVL_node_height(TAVLNode *node) {
    if (node == NULL) {
        return 0;
    }

    return node->height;
}

/**
 * @brief Calculates the balance factor of an AVL node
 *
 * The balance factor is the right subtree height minus the left subtree
 * height. A negative value means the left subtree is taller.
 *
 * @param node Node whose balance factor is calculated; may be NULL
 *
 * @return Balance factor, or 0 if node is NULL
 */
static int get_balance(TAVLNode *node) {
    if (node == NULL) {
        return 0;
    }
    return AVL_node_height(node->right) - AVL_node_height(node->left);
}

/**
 * @brief Updates a node height using the heights of its children
 *
 * Does nothing if node is NULL.
 *
 * @param node Node whose height is updated
 */
static void update_node_height(TAVLNode *node) {
    if (node == NULL) {
        return;
    }
    node->height = max(AVL_node_height(node->right), AVL_node_height(node->left)) + 1;
}

/**
 * @brief Performs a right rotation around a subtree root
 *
 * Updates the heights of both nodes affected by the rotation.
 *
 * @param root Root of the subtree; must have a left child
 *
 * @return New root of the rotated subtree
 */
static TAVLNode *right_rotate(TAVLNode *root) {
    // Move the left child up and attach its right subtree to the old root.
    TAVLNode *newRoot = root->left;
    root->left = newRoot->right;
    newRoot->right = root;
    // Update the old root first because it is now a child of newRoot.
    update_node_height(root);
    update_node_height(newRoot);

    return newRoot;
}

/**
 * @brief Performs a left rotation around a subtree root
 *
 * Updates the heights of both nodes affected by the rotation.
 *
 * @param root Root of the subtree; must have a right child
 *
 * @return New root of the rotated subtree
 */
static TAVLNode *left_rotate(TAVLNode *root) {
    // Move the right child up and attach its left subtree to the old root.
    TAVLNode *newRoot = root->right;
    root->right = newRoot->left;
    newRoot->left = root;
    // Update the old root first because it is now a child of newRoot.
    update_node_height(root);
    update_node_height(newRoot);

    return newRoot;
}

/**
 * @brief Recursively searches an AVL subtree for a key
 *
 * Compares keys using strcmp().
 *
 * @param root Root of the subtree; may be NULL
 * @param k Symbol key to search for; must not be NULL
 *
 * @return Pointer to the matching node, or NULL if the key is not found
 */
static TAVLNode *search_AVL_tree(TAVLNode *root, TKey k) {
    if (root == NULL) {
        return NULL;
    }

    int cmp = strcmp(k, root->key);
    if (cmp == 0) {
        return root;
    }
    if (cmp > 0) {
        return search_AVL_tree(root->right, k);
    }
    return search_AVL_tree(root->left, k);
}

/**
 * @brief Creates an AVL leaf node containing a symbol
 *
 * Stores the key pointer directly and copies the symbol data by value.
 * Initializes the node height to 1 and both child pointers to NULL.
 *
 * @param k Symbol key to store
 * @param d Symbol data to store
 *
 * @return Pointer to the newly allocated node
 */
static TAVLNode *create_AVL_node(TKey k, TData d) {
    TAVLNode *new = (TAVLNode *)malloc(sizeof(TAVLNode));
    if (new == NULL) {
        throw_fatal_error(99, "Could not allocate memory!\n");
    }
    new->key = k;
    new->data = d;
    new->height = 1;
    new->right = NULL;
    new->left = NULL;

    return new;
}

/**
 * @brief Inserts a symbol into an AVL subtree
 *
 * Creates a leaf for a new key, updates subtree heights and restores
 * balance using rotations. If the key already exists, replaces only
 * its symbol data without freeing resources referenced by the old data.
 *
 * @param root Root of the subtree; may be NULL
 * @param k Symbol key; must not be NULL
 * @param d Symbol data to store
 *
 * @return Root of the subtree after insertion and balancing
 */
static TAVLNode *insert_AVL_node(TAVLNode *root, TKey k, TData d) {
    if (root == NULL) {
        return create_AVL_node(k, d);
    }

    int cmp = strcmp(k, root->key);
    if (cmp == 0) {
        root->data = d;
    } else {
        if (cmp > 0) {
            root->right = insert_AVL_node(root->right, k, d);
        } else {
            root->left = insert_AVL_node(root->left, k, d);
        }
        update_node_height(root);
        int balance = get_balance(root);
        // A difference greater than one level means the node needs balancing.
        if (balance < -1) {
            int sonBalance = get_balance(root->left);
            if (sonBalance >= 1) {
                // The left child leans right, so rotate it before rotating root.
                root->left = left_rotate(root->left);
            }
            root = right_rotate(root);
        } else if (balance > 1) {
            int sonBalance = get_balance(root->right);
            if (sonBalance <= -1) {
                // The right child leans left, so rotate it before rotating root.
                root->right = right_rotate(root->right);
            }
            root = left_rotate(root);
        }
    }

    return root;
}

/**
 * @brief Removes a symbol from an AVL subtree
 *
 * If the node has two children, copies the key and data from the node
 * with the smallest key in its right subtree, then deletes that node.
 * Updates subtree heights and restores balance after deletion.
 * Leaves the subtree unchanged if the key is not found.
 *
 * @param root Root of the subtree; may be NULL
 * @param k Symbol key to remove; must not be NULL
 *
 * @return Root of the subtree after deletion and balancing
 *
 * @note Frees removed nodes and function parameter type arrays, but does
 *       not free keys, definition AST nodes or function local scopes.
 */
static TAVLNode *AVL_delete(TAVLNode *root, TKey k) {
    if (root == NULL) {
        return NULL;
    }

    int cmp = strcmp(k, root->key);
    if (cmp > 0) {
        root->right = AVL_delete(root->right, k);
    } else if (cmp < 0) {
        root->left = AVL_delete(root->left, k);
    } else {
        if (root->data.kind == SYMBOL_FUNCTION) {
            // Free the function parameter type array before removing its data.
            free(root->data.data.function.param_types);
        }

        if ((root->right == NULL) && (root->left == NULL)) {
            free(root);
            return NULL;
        }

        if ((root->right == NULL) || (root->left == NULL)) {
            // The only child takes the place of the removed node.
            TAVLNode *onlyChild;
            if (root->left == NULL) {
                onlyChild = root->right;
            } else {
                onlyChild = root->left;
            }
            free(root);
            return onlyChild;
        } else {
            // The smallest key on the right keeps the tree ordered after replacement.
            TAVLNode *min = AVL_min(root->right);
            root->key = min->key;
            root->data = min->data;

            if (min->data.kind == SYMBOL_FUNCTION) {
                // Set to NULL so deleting min does not free the parameter types now used by root.
                min->data.data.function.param_types = NULL;
            }

            root->right = AVL_delete(root->right, min->key);
        }
    }

    // Removing a node can change the height and balance of its parent nodes.
    update_node_height(root);
    int balance = get_balance(root);
    if (balance < -1) {
        int sonBalance = get_balance(root->left);
        if (sonBalance >= 1) {
            // The left child leans right, so rotate it before rotating root.
            root->left = left_rotate(root->left);
        }
        root = right_rotate(root);
    } else if (balance > 1) {
        int sonBalance = get_balance(root->right);
        if (sonBalance <= -1) {
            // The right child leans left, so rotate it before rotating root.
            root->right = right_rotate(root->right);
        }
        root = left_rotate(root);
    }

    return root;
}

/**
 * @brief Recursively frees all nodes in an AVL subtree
 *
 * Frees child subtrees before the current node. For function symbols,
 * also frees the parameter type array. Does not free keys, definition
 * AST nodes or function local scopes.
 *
 * @param root Root of the subtree to destroy; may be NULL
 */
static void destroy_AVL_tree(TAVLNode *root) {
    if (root == NULL) {
        return;
    }
    // Free children before their parent so their pointers remain accessible.
    destroy_AVL_tree(root->right);
    destroy_AVL_tree(root->left);
    if (root->data.kind == SYMBOL_FUNCTION) {
        free(root->data.data.function.param_types);
    }
    free(root);
}

SymTable *create_symtable(void) {
    SymTable *table = (SymTable *)malloc(sizeof(SymTable));
    if (table == NULL) {
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
    if (node != NULL) {
        return &node->data;
    }
    return NULL;
}

TData *search_scope(Scope *scope, TKey k) {
    while (scope != NULL) {
        TData *data = search(scope->symbols, k);

        if (data != NULL) {
            return data;
        }

        // Check the parent scope only if the name was not found here.
        scope = scope->parent;
    }

    return NULL;
}

void destroy_symtable(SymTable *symtable) {
    destroy_AVL_tree(symtable->root);
    free(symtable);
}

Scope *create_scope(Scope *parent) {
    Scope *scope = (Scope *)malloc(sizeof(Scope));
    if (scope == NULL) {
        throw_fatal_error(99, "Could not allocate memory!\n");
    }
    scope->parent = parent;
    scope->symbols = create_symtable();

    return scope;
}

void destroy_scope(Scope *scope) {
    destroy_symtable(scope->symbols);
    free(scope);
}
