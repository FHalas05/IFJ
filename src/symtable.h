//
// Created by Filip Halaš on 18/10/2026.
//

#ifndef IFJ_SYMTABLE_H
#define IFJ_SYMTABLE_H

#include "ast.h"

typedef struct SymTable SymTable;

typedef struct Scope {
    SymTable *symbols;
    struct Scope *parent;
} Scope;

typedef char *TKey;

typedef enum {
    SYMBOL_FUNCTION,
    SYMBOL_VARIABLE,
} SymbolKind;

typedef struct {
    DataType type;
    bool is_mutable;
    size_t variable_id;
} VariableData;

typedef struct {
    DataType return_type;
    const DataType *param_types;
    size_t param_count;
    bool is_builtin;
    ASTNode *definition_node;
    Scope *local_scope;
} FunctionData;

typedef struct {
    SymbolKind kind;

    union {
        FunctionData function;
        VariableData variable;
    } data;
} TData;

SymTable *create_symtable(void);
void insert(SymTable *symtable, TKey k, TData d);
void symtable_delete(SymTable *symtable, TKey k);
TData *search(SymTable *symtable, TKey k);
TData *search_scope(Scope *scope, TKey k);
void destroy_symtable(SymTable *symtable);
Scope *create_scope(Scope *parent);
void destroy_scope(Scope *scope);

#endif // IFJ_SYMTABLE_H
