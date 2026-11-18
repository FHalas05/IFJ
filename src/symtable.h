/**
 * @file symtable.h
 * @brief Symbol table and nested scope interface
 *
 * Defines variable and function symbol data and operations for managing
 * AVL-based symbol tables and searching nested scopes.
 *
 * @author Filip Halaš (xhalasf00)
 * @date 18.10.2026
 */

#ifndef IFJ_SYMTABLE_H
#define IFJ_SYMTABLE_H

#include "ast.h"

/**
 * @brief Symbol table implemented as an AVL tree
 *
 * The internal representation is defined in symtable.c.
 */
typedef struct SymTable SymTable;

/**
 * @brief Scope containing a symbol table and a reference to its parent
 */
typedef struct Scope {
    SymTable *symbols; //Symbols declared in this scope
    struct Scope *parent; //Parent scope, or NULL for the outermost scope
} Scope;

/**
 * @brief Symbol name used as a key in the symbol table
 */
typedef char *TKey;

/**
 * @brief Kind of symbol stored in the symbol table
 */
typedef enum {
    SYMBOL_FUNCTION,
    SYMBOL_VARIABLE,
} SymbolKind;

/**
 * @brief Semantic information associated with a variable
 */
typedef struct {
    DataType type;      //Data type of the variable
    bool is_mutable;    //Whether the variable can be reassigned
    size_t variable_id; //Identifier assigned to the variable
} VariableData;

/**
 * @brief Semantic information associated with a function
 */
typedef struct {
    DataType return_type;        //Return type of the function
    DataType *param_types;       //Parameter types in declaration order
    size_t param_count;          //Number of function parameters
    bool is_builtin;             //Whether the function is built in
    ASTNode *definition_node;     //Function definition in the AST, or NULL for built-in functions
    Scope *local_scope;          //Optional scope reference; not freed by the symbol table
} FunctionData;

/**
 * @brief Symbol data interpreted according to the symbol kind
 *
 * Only the union member matching kind should be used.
 */
typedef struct {
    SymbolKind kind;

    union {
        FunctionData function;
        VariableData variable;
    } data;
} TData;

/**
 * @brief Creates an empty symbol table
 *
 * @return Pointer to the newly allocated symbol table
 */
SymTable *create_symtable(void);

/**
 * @brief Inserts a symbol or replaces the data of an existing symbol
 *
 * Stores the key pointer directly and copies the symbol data by value.
 * If the key already exists, its data is replaced and its original key
 * pointer is retained. Resources referenced by the old data are not freed.
 *
 * @param symtable Symbol table to modify; must not be NULL
 * @param k Symbol key; must not be NULL
 * @param d Data to associate with the key
 *
 * @note A newly stored key must remain valid and unchanged while it is
 *       used by the table.
 * @note Parameter type arrays stored in function symbols are freed when
 *       the symbol is deleted or when the table is destroyed, and must be
 *       suitable for free().
 */
void insert(SymTable *symtable, TKey k, TData d);

/**
 * @brief Removes a symbol from the table
 *
 * Does nothing if the key is not found. Frees the removed tree node
 * and its function parameter type array, but does not free its key,
 * definition AST node or function local scope.
 *
 * @param symtable Symbol table to modify; must not be NULL
 * @param k Symbol key to remove; must not be NULL
 */
void symtable_delete(SymTable *symtable, TKey k);

/**
 * @brief Searches for a symbol in a single symbol table
 *
 * @param symtable Symbol table to search; must not be NULL
 * @param k Symbol key; must not be NULL
 *
 * @return Pointer to the stored symbol data, or NULL if the key is not found
 *
 * @note The returned pointer belongs to the table and must not be freed.
 *       Deleting symbols or destroying the table may invalidate it.
 */
TData *search(SymTable *symtable, TKey k);

/**
 * @brief Searches for a symbol in a scope and its parent scopes
 *
 * Searches the supplied scope first and then follows parent pointers.
 * Returns the first match, allowing inner declarations to shadow
 * declarations in parent scopes.
 *
 * @param scope Scope where the search begins; may be NULL
 * @param k Symbol key; must not be NULL
 *
 * @return Pointer to the nearest matching symbol data, or NULL if not found
 *
 * @note The returned pointer belongs to a symbol table and must not be freed.
 *       Deleting symbols or destroying that table may invalidate it.
 */
TData *search_scope(Scope *scope, TKey k);

/**
 * @brief Destroys a symbol table and all its tree nodes
 *
 * Frees parameter type arrays belonging to stored function symbols.
 * Does not free symbol keys, function definition AST nodes or function
 * local scopes.
 *
 * @param symtable Symbol table to destroy; must not be NULL
 */
void destroy_symtable(SymTable *symtable);

/**
 * @brief Creates a scope with an empty symbol table
 *
 * @param parent Parent scope, or NULL for an outermost scope
 *
 * @return Pointer to the newly allocated scope
 */
Scope *create_scope(Scope *parent);

/**
 * @brief Destroys a scope and its symbol table
 *
 * Does not destroy its parent scope or any nested scopes.
 *
 * @param scope Scope to destroy; must not be NULL
 */
void destroy_scope(Scope *scope);

#endif // IFJ_SYMTABLE_H
