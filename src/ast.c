/**
 * @file ast.c
 * @brief Functions for working with the AST
 *
 * Making nodes, adding nodes to arrays and freeing the whole tree.
 * The functions are described in ast.h.
 *
 * @author Jakub Juřík (xjurikj00)
 * @date 6.10.2026
 */

#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

// Adds one node to the end of a node array (see ast.h)
void ast_array_append(ASTNode ***array, size_t *count, size_t *capacity, ASTNode *new_item) {
    // The array is full -> make it bigger
    if (*count == *capacity) {
        size_t new_capacity;
        if (*capacity == 0) {
            new_capacity = 4; // first item: start with space for 4 items
        } else {
            new_capacity = *capacity * 2; // otherwise make it 2x bigger
        }
        ASTNode **new_array = realloc(*array, new_capacity * sizeof(ASTNode *));
        if (new_array == NULL) {
            fprintf(stderr, "reallocation error\n");
            exit(99);
        }
        *array = new_array;
        *capacity = new_capacity;
    }

    // Put the new node at the end
    (*array)[*count] = new_item;
    (*count)++;
}

// Makes an empty node of the given type (see ast.h)
ASTNode *create_node(NodeType type) {
    // calloc sets all data to 0 / NULL
    ASTNode *one_node = calloc(1, sizeof(ASTNode));
    if (one_node == NULL) {
        fprintf(stderr, "allocation error\n");
        exit(99);
    }
    one_node->type = type;
    one_node->data_type = TYPE_UNKNOWN; // the semantic analysis sets the real type
    return one_node;
}

// Frees a node and all its children (see ast.h)
void free_ast(ASTNode *node) {
    if (node == NULL) {
        return;
    }

    // First free what the node owns: child nodes (free_ast),
    // arrays of children and strings (free)
    switch (node->type) {
    case NODE_PROGRAM:
        for (size_t i = 0; i < node->data.program.count; i++) {
            free_ast(node->data.program.functions[i]);
        }
        free(node->data.program.functions);
        break;
    case NODE_BLOCK:
        for (size_t i = 0; i < node->data.block.count; i++) {
            free_ast(node->data.block.statements[i]);
        }
        free(node->data.block.statements);
        break;
    case NODE_FUNCTION_DEF:
        free(node->data.function_def.name);
        for (size_t i = 0; i < node->data.function_def.param_count; i++) {
            free_ast(node->data.function_def.parameters[i]);
        }
        free(node->data.function_def.parameters);
        free_ast(node->data.function_def.body);
        break;
    case NODE_FUNCTION_CALL:
        free(node->data.function_call.name);
        for (size_t i = 0; i < node->data.function_call.arg_count; i++) {
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
    case NODE_UNOP:
        free_ast(node->data.un_op.operand);
        break;
    case NODE_IF:
        free_ast(node->data.if_stmt.condition);
        free_ast(node->data.if_stmt.else_block); // can be NULL (no else)
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
        free_ast(node->data.return_stmt.value); // can be NULL (return without a value)
        break;
    case NODE_IDENTIFIER:
        free(node->data.identifier.name);
        break;

    // Number and Boolean literals own nothing
    case NODE_INT_LITERAL:
        break;
    case NODE_DOUBLE_LITERAL:
        break;
    case NODE_STRING_LITERAL:
        free(node->data.string_lit.value);
        break;
    case NODE_BOOL_LITERAL:
        break;
    case NODE_PARAM:
        free(node->data.param.name);
        break;
    default:
        break;
    }
    // At the end free the node itself
    free(node);
}
