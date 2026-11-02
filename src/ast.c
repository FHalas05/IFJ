#include "ast.h"
#include <stdio.h>
#include <stdlib.h>


void ast_array_append(ASTNode ***array, size_t *count, size_t *capacity, ASTNode *new_item){
    if (*count == *capacity){
        size_t new_capacity;
        if (*capacity == 0){
            new_capacity = 4;
        }
        else{
            new_capacity = *capacity * 2;
        }    
        ASTNode** new_array = realloc(*array, new_capacity * sizeof(ASTNode*));
        if(new_array == NULL){
            fprintf(stderr, "reallocation error\n");
            exit(99);
        }
        *array = new_array;
        *capacity = new_capacity;
    }

    (*array)[*count] = new_item;
    (*count)++;
}

ASTNode* create_node(NodeType type){
    ASTNode* one_node = calloc(1, sizeof(ASTNode));
    if(one_node == NULL){
        fprintf(stderr, "allocation error\n");
        exit(99);
    }
    one_node->type = type;
    one_node->data_type = TYPE_UNKNOWN;
    return one_node;
}

void free_ast(ASTNode *node){
    if (node == NULL){
        return;
    }

    switch (node->type){
        case NODE_PROGRAM:
            for (size_t i = 0; i < node->data.program.count; i++){
                free_ast(node->data.program.functions[i]);
            }
            free(node->data.program.functions);
            break;
        case NODE_BLOCK:
            for(size_t i = 0; i < node->data.block.count; i++){
                free_ast(node->data.block.statements[i]);
            }
            free(node->data.block.statements);
            break;
        case NODE_FUNCTION_DEF:
            free(node->data.function_def.name);
            for(size_t i = 0; i < node->data.function_def.param_count; i++){
                free_ast(node->data.function_def.parameters[i]);
            }
            free(node->data.function_def.parameters);
            free_ast(node->data.function_def.body);
            break;
        case NODE_FUNCTION_CALL:
            free(node->data.function_call.name);
            for(size_t i = 0; i < node->data.function_call.arg_count; i++){
                free_ast(node->data.function_call.arguments[i]);
            }
            free(node->data.function_call.arguments);
            break;
        case NODE_DEF_VAR:
            free(node->data.def_var.name);
            free_ast(node->data.def_var.value);
            break;
        case NODE_BINOP:
            free_ast(node->data.bin_op.left);
            free_ast(node->data.bin_op.right);
            break;
        case NODE_IF:
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.else_block);
            free_ast(node->data.if_stmt.then_block);
            break;
        case NODE_WHILE:
            free_ast(node->data.while_loop.body);
            free_ast(node->data.while_loop.condition);
            break;
        case NODE_ASSIGN:
            free(node->data.assign.name);
            free_ast(node->data.assign.value);
            break;
        case NODE_RETURN:
            free_ast(node->data.return_stmt.value);
            break;
        case NODE_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        case NODE_INT_LITERAL:
            break;
        case NODE_DOUBLE_LITERAL:
            break;
        case NODE_STRING_LITERAL:
            free(node->data.string_lit.value);
        case NODE_BOOL_LITERAL:
            break;
        case NODE_PARAM:
            free(node->data.param.name);
            break;
        default:
            break;
    }
    free(node);
}