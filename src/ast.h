

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
    NODE_NUMBER                 //number - int
    
} NodeType;

typedef struct ASTNode{
    NodeType type;

    union{
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
        } function_def;

        struct{
            char *name;
            struct ASTNode **arguments;
            size_t arg_count;
        } function_call;

        struct{
            char *name;
            bool var;                        //if not -> val
            struct ASTNode *value;
        } def_var;

        struct{
            int operation;
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

        char *identifier;

        int number;
    } data;

} ASTNode;

ASTNode* create_node(NodeType type);
void free_ast(ASTNode *node);

#endif