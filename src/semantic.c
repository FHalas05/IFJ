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

static void collect_functions(SemContext *ctx, ASTNode *program){
    ctx->functions = create_symtable();
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

void semantic_analyse (ASTNode *program){
    SemContext ctx = {0};
    collect_functions(&ctx, program);
    destroy_symtable(ctx.functions);
}
