

#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <stdbool.h>

//Everything that can be in the program
typedef enum{
    NODE_PROGRAM                //program
    NODE_BLOCK                  //block of operations
    NODE_FUNCTION_DEF           //function definition
    NODE_FUNCTION_CALL          //call function
    NODE_DEF_VAR                //definition - val, var
    NODE_BINOP                  //binnary operation
    NODE_IF                     //if cycle
    NODE_WHILE                  //while_cycle
    NODE_ASSIGN                 //assignment (x=1)
    NODE_RETURN                 //return
    NODE_IDENTIFIER             //Variable name in expression
    NODE_INT_LITERAL            //intiger        
    NODE_DOUBLE_LITERAL         //double                 
    NODE_STRING_LITERAL         //string
    NODE_BOOL_LITERAL           //boolean
    NODE_UNIT_LITERAL           //unit
} NodeType;

//operations for numbers
typedef enum{
    OP_ADD, OP_SUBB, OP_MUL, OP_DIV, 
    OP_LS, OP_LE, OP_GT, OP_GE,
    OP_EQ, OP_NEQ,
    OP_UNARY_MINUS, OP_UNARY_PLUS
} Operator;

//structures for everything - union = only one structure at a time
typedef struct ASTNode{
    NodeType type;
    int line;
    int column;

    union{
        struct{
            struct ASTNode **functions;
            size_t count;
            size_t capacity;
        } program;

        struct {
            struct ASTNode **statements;
            size_t count;
            size_t capacity;
        } block;

        struct {
            char *name;
            struct ASTNode **parameters;
            size_t param_count;
            struct ASTNode *body;
            char *return_type;
            bool has_explicit_return_type;
        } function_def;

        struct{
            char *name;
            struct ASTNode **arguments;
            size_t arg_count;
        } function_call;

        struct{
            char *name;
            bool var;                                   //if not -> val
            struct ASTNode *value;
        } def_var;

        struct{
            Operator operation;
            struct ASTNode *left;
            struct ASTNode *right;
        } bin_op;

        struct{
            struct ASTNode *condition;
            struct ASTNode *then_cond;
            struct ASTNode *else_cond;
        } if_stmt;

        struct{
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_loop;

        struct{
            char *name;
            struct ASTNode *value;
        } assign;

        struct{
            struct ASTNode *value;
        } return_stmt;

        struct{
            char *name;
        } identifier;
        
        struct{
            long value;
        } int_lit;

        struct{
            double value;
        } double_lit;

        struct{
            char *value;
        } string_lit;

        struct {
            bool value;
        } bool_lit;

    } data;

} ASTNode;

ASTNode* create_node(NodeType type);
void free_ast(ASTNode *node);

void ast_array_append(ASTNode ***array, size_t *count, size_t *capacity, ASTNode *new_item);
 
#endif