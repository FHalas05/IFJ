#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "semantic.h"
#include "symtable.h"
#include "library.h"

static void add_builtin(SemContext *ctx, char *name, DataType return_type, size_t param_count, DataType *params);

static void collect_functions(SemContext *ctx, ASTNode *program);

static void check_block(SemContext *ctx, ASTNode *block);

static void check_statement(SemContext *ctx, ASTNode *stmt);

static void check_function(SemContext *ctx, ASTNode *func);

bool types_compatible(DataType expected, DataType actual);

static DataType check_expression(SemContext *ctx, ASTNode *expr);

void semantic_analyze (ASTNode *program);

static bool is_numeric(DataType t);


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
            else if ((func->data.function_def.has_explicit_return_type == true) && (func->data.function_def.return_type != TYPE_UNIT)){
                throw_fatal_error(6, "@main must return Unit");
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
            DataType value_type = check_expression(ctx, stmt->data.def_var.value);
            if (value_type == TYPE_BOOL){
                throw_fatal_error(6, "Value cannot be true/false");
            }
            DataType var_type = stmt->data.def_var.type;
            if (value_type != TYPE_UNKNOWN){
                if (var_type == TYPE_UNKNOWN){
                    var_type = value_type;
                }
                else if(!types_compatible(var_type, value_type)){
                            throw_fatal_error(6,"value and var are not compatible");
                }
            }
            
            char *name = stmt->data.def_var.name;
            if (search(ctx->current->symbols, name) != NULL){
                throw_fatal_error(4, "VAL/VAR is out of range");
            }
            TData d = {0};
            d.kind = SYMBOL_VARIABLE;
            d.data.variable.type = var_type;
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
            stmt->data.function_call.is_builtin = f->data.function.is_builtin;
            break;
        }
        case NODE_RETURN:{
            if (ctx->current_function->data.function_def.has_explicit_return_type == false){
                throw_fatal_error(6, "Wrong return type");
            }  
            else if ((stmt->data.return_stmt.value == NULL) && (ctx->current_function->data.function_def.return_type != TYPE_UNIT)){
                throw_fatal_error(6, "Wrong return type");
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
    ASTNode *body = func->data.function_def.body;
    for (size_t i = 0; i < body->data.block.count ; i++){
        check_statement(ctx, body->data.block.statements[i]);
    }
    destroy_scope(ctx->current);
    ctx->current = NULL;
}

bool types_compatible(DataType expected, DataType actual){
    if (actual == TYPE_BOOL){
        return false;
    }
    else if (expected == actual){
        return true;
    }
    else if (expected == TYPE_ANY){
        return true;
    }
    else if ((expected == TYPE_DOUBLE ) && (actual == TYPE_INT)){
        return  true;
    }
    return false;
}

static DataType check_expression(SemContext *ctx, ASTNode *expr){
    if (expr == NULL){
        return TYPE_UNIT;
    }
    DataType result = TYPE_UNKNOWN;
    switch (expr->type){
        case NODE_INT_LITERAL:
            result = TYPE_INT;
            break;
        case NODE_DOUBLE_LITERAL:
            result = TYPE_DOUBLE;
            break;
        case NODE_STRING_LITERAL:
            result = TYPE_STRING;
            break;  
        case NODE_UNIT_LITERAL:
            result = TYPE_UNIT;
            break;
        case NODE_IDENTIFIER:{
            TData *var = search_scope(ctx->current, expr->data.identifier.name);
            if (var == NULL){
                throw_fatal_error(3, "Undefined variable");
            }  
            result = var->data.variable.type;
            break;
        }
        case NODE_BINOP:{
            DataType a = check_expression(ctx, expr->data.bin_op.left);
            DataType b = check_expression(ctx, expr->data.bin_op.right);
            if ((a == TYPE_UNKNOWN) || (b == TYPE_UNKNOWN)){
                result = TYPE_UNKNOWN;
                break;
            }
            switch(expr->data.bin_op.operation){
                case OP_ADD:
                    if ((is_numeric(a)) && (is_numeric(b))){
                        if ((a == TYPE_INT) && (b == TYPE_INT)){
                            result = TYPE_INT;
                        }
                        else{
                            result = TYPE_DOUBLE;
                        }
                    }
                    else if((a == TYPE_STRING) && (b == TYPE_STRING)){
                        result = TYPE_STRING;
                    }
                    else{
                        throw_fatal_error(6, "Operator '+' requires two numbers or two strings");
                    }
                    break;
                case OP_SUB:
                case OP_MUL:
                case OP_DIV:
                    if ((is_numeric(a)) && (is_numeric(b))){
                        if ((a == TYPE_INT) && (b == TYPE_INT)){
                            result = TYPE_INT;
                        }
                        else{
                            result = TYPE_DOUBLE;
                        }
                    }
                    else{
                        throw_fatal_error(6, "Arithmetic operator requires numeric operands");
                    }
                    break;
                case OP_GR:
                case OP_GR_EQ:
                case OP_LESS:
                case OP_LESS_EQ:
                    if (((is_numeric(a)) && (is_numeric(b))) || ((a == TYPE_STRING) && (b == TYPE_STRING))){
                        result = TYPE_BOOL;
                    }
                    else{
                        throw_fatal_error(6, "Relational operator requires two numbers or two strings");
                    }
                    break;
                case OP_EQ:
                case OP_NOT_EQ:
                    if ((is_numeric(a)) && (is_numeric(b))){
                        result = TYPE_BOOL;
                    }
                    else if ((a == b) && (a != TYPE_BOOL)){
                        result = TYPE_BOOL;
                    }
                    else{
                        throw_fatal_error(6, "Equality operator requires compatible operands");
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    expr->data_type = result;
        return result;
}

static bool is_numeric(DataType t){
    if ((t == TYPE_INT ) || (t == TYPE_DOUBLE)){
        return true;
    }
    return false;
}



void semantic_analyze (ASTNode *program){
    SemContext ctx = {0};
    collect_functions(&ctx, program);
    for (size_t i = 0; i < program->data.program.count; i++){
        ASTNode *func = program->data.program.functions[i];
        check_function(&ctx, func);
    }
    destroy_symtable(ctx.functions);
}
