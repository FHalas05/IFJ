#ifndef IFJ_SEMANTIC_H
#define IFJ_SEMANTIC_H

#include <stddef.h>
#include <stdbool.h>

#include "ast.h"
#include "symtable.h"

typedef struct{
    SymTable *functions;
    Scope *current;
    size_t next_var_id;
    ASTNode *current_function;
}SemContext;

void semantic_analyse(ASTNode *program);

bool types_compatible(DataType expected, DataType actual);

#endif