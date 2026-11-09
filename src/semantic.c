#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "semantic.h"
#include "symtable.h"
#include "library.h"

typedef struct{
    SymTable *functions;
    Scope *current;
    size_t next_var_id;
    ASTNode *current_function;
}SemContext;

static void add_builtin(SemContext *ctx, char *name, DataType return_type, size_t param_count, DataType *params){
    DataType *types = NULL;
    if (param_count > 0){
        types = malloc (param_count * sizeof(DataType));
        if (types == NULL){
            throw_fatal_error(99, "Memory allocation error");
        }
        for (size_t i = 0; i < param_count; i++){
            types[i] = params[i];
        }
    }
    TData d = {0};
    d.kind = SYMBOL_FUNCTION;
    d.data.function.return_type = return_type;
    d.data.function.param_types = types;
    d.data.function.param_count = param_count;
    d.data.function.is_builtin = true;
    d.data.function.definition_node = NULL;
    insert(ctx->functions, name, d);
}

static void collect_functions(SemContext *ctx, ASTNode *program){
    ctx->functions = create_symtable();
    add_builtin(ctx, "print", TYPE_UNIT, 1, (DataType[]){TYPE_ANY});
    add_builtin(ctx, "readLine", TYPE_STRING, 0, NULL);
    add_builtin(ctx, "readInt", TYPE_INT, 0, NULL);
    add_builtin(ctx, "readDouble", TYPE_DOUBLE, 0, NULL);
    add_builtin(ctx, "toStr", TYPE_STRING, 1, (DataType[]){TYPE_ANY});
    add_builtin(ctx, "toInt", TYPE_INT, 1, (DataType[]){TYPE_DOUBLE});
    add_builtin(ctx, "strLength", TYPE_INT, 1, (DataType[]){TYPE_STRING});
    add_builtin(ctx, "subStr", TYPE_STRING, 3, (DataType[]){TYPE_STRING, TYPE_INT, TYPE_INT});
    add_builtin(ctx, "ord", TYPE_INT, 2, (DataType[]){TYPE_STRING, TYPE_INT});
    add_builtin(ctx, "chr", TYPE_STRING, 1,(DataType[]){TYPE_INT});
    int main_count = 0;
    for (size_t i = 0; i < program->data.program.count; i++){
        ASTNode *func = program->data.program.functions[i];
        char *name = func->data.function_def.name;

        if(search(ctx->functions, name) != NULL){
            throw_fatal_error(4, "Function redefinition");
        }

        for (size_t j = 0; j < func->data.function_def.param_count; j++){
            for (size_t k = j+1; k < func->data.function_def.param_count; k++){
                char *name_1 = func->data.function_def.parameters[j]->data.param.name;
                char* name_2 = func->data.function_def.parameters[k]->data.param.name;
                if (strcmp(name_1, name_2) == 0){
                    throw_fatal_error(4, "Duplicate parameter name");
                }
            }
        }

        if (func->data.function_def.is_main){
            main_count ++;
            if (main_count > 1){
                throw_fatal_error(4, "Only one @main is allowed");
            }
            else if (func->data.function_def.param_count > 0){
                throw_fatal_error(5, "@main must have no parameters");
            }
        } 

        DataType *types = NULL;
        if (func->data.function_def.param_count > 0){
             types  = malloc(func->data.function_def.param_count * sizeof(DataType));
            if (types == NULL){
                throw_fatal_error(99, "Memory allocation error");
            }
            for (size_t j = 0; j < func->data.function_def.param_count; j++){
                types[j] = func->data.function_def.parameters[j]->data.param.data;
            }

            
        }

        TData d = {0};
            d.kind = SYMBOL_FUNCTION;
            d.data.function.return_type = func->data.function_def.return_type;
            d.data.function.param_types = types;
            d.data.function.param_count = func->data.function_def.param_count;
            d.data.function.is_builtin = false;
            d.data.function.definition_node = func;
            insert(ctx->functions, name, d);    
    }
    if (main_count == 0){
                throw_fatal_error(3, "@main not found");
        }
}

static void check_statement(SemContext *ctx, ASTNode *stmt);

static void check_block(SemContext *ctx, ASTNode *block){  
    ctx->current = create_scope (ctx->current);
    for (size_t i = 0; i < block->data.block.count; i++){
        ASTNode *stmt = block->data.block.statements[i];
        check_statement(ctx, stmt);
    }
    Scope *parent = ctx->current->parent;
    destroy_scope(ctx->current);
    ctx->current = parent;
}

static void check_statement(SemContext *ctx, ASTNode *stmt){
    if (stmt == NULL){
        return;
    }
    switch (stmt->type){
        case NODE_DEF_VAR:{
            char *name = stmt->data.def_var.name;
            if (search(ctx->current->symbols, name) != NULL){
                throw_fatal_error(4, "VAL/VAR is out of range");
            }
            TData d = {0};
            d.kind = SYMBOL_VARIABLE;
            d.data.variable.type = stmt->data.def_var.type;
            d.data.variable.is_mutable = stmt->data.def_var.var;
            d.data.variable.variable_id = ctx->next_var_id++;

            insert(ctx->current->symbols, name, d);
            break;
        }

        case NODE_ASSIGN:{
            char *name = stmt->data.assign.name;
            TData *var = search_scope(ctx->current, name);
            if (var == NULL){
                throw_fatal_error(3, "Undefined variable");
            }
            else if (!var->data.variable.is_mutable){
                throw_fatal_error(7, "Assignment to val / parameter");
            }
            break;
        }

        case NODE_IF:{
            check_block(ctx, stmt->data.if_stmt.then_block);
            check_block(ctx, stmt->data.if_stmt.else_block);
            break;
        }

        case NODE_WHILE:{
            check_block(ctx, stmt->data.while_loop.body);
            break;
        }
        case NODE_FUNCTION_CALL:{
            TData *f = search(ctx->functions, stmt->data.function_call.name);
            if (f == NULL){
                throw_fatal_error(3, "Undefined function");
            }
            else if (f->data.function.param_count != stmt->data.function_call.arg_count){
                throw_fatal_error(5, "Wrong number of arguments");
            }
            break;
        }
        default:
            break;
    }
}

static void check_function(SemContext *ctx, ASTNode *func){
    ctx->current_function = func;
    ctx->current = create_scope(NULL);
    for (size_t i = 0; i < func->data.function_def.param_count; i++){
        ASTNode *param = func->data.function_def.parameters[i];
        TData d = {0};
        d.kind = SYMBOL_VARIABLE;
        d.data.variable.type = param->data.param.data;
        d.data.variable.is_mutable = false;
        d.data.variable.variable_id = ctx->next_var_id++;
        insert(ctx->current->symbols, param->data.param.name, d);
    }
    check_block(ctx, func->data.function_def.body);
    destroy_scope(ctx->current);
    ctx->current = NULL;
}

void semantic_analyse (ASTNode *program){
    SemContext ctx = {0};
    collect_functions(&ctx, program);
    for (size_t i = 0; i < program->data.program.count; i++){
        ASTNode *func = program->data.program.functions[i];
        check_function(&ctx, func);
    }
    destroy_symtable(ctx.functions);
}
