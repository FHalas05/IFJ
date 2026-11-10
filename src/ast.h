

#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <stdbool.h>

typedef enum{
    TYPE_UNKNOWN,
    TYPE_ANY,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_BOOL
} DataType;

//Everything that can be in the program
typedef enum{
    NODE_PROGRAM,                //program
    NODE_BLOCK,                  //block of operations
    NODE_FUNCTION_DEF,           //function definition
    NODE_FUNCTION_CALL,          //call function
    NODE_DEF_VAR,                //definition - val, var
    NODE_BINOP,                  //binary operation
    NODE_UNOP,                   //unary operation
    NODE_IF,                     //if cycle
    NODE_WHILE,                  //while_cycle
    NODE_ASSIGN,                 //assignment (x=1)
    NODE_RETURN,                 //return
    NODE_IDENTIFIER,             //Variable name in expression
    NODE_INT_LITERAL,            //intiger        
    NODE_DOUBLE_LITERAL,         //double                 
    NODE_STRING_LITERAL,         //string
    NODE_BOOL_LITERAL,           //boolean
    NODE_UNIT_LITERAL,           //unit
    NODE_PARAM                   //parameter
} NodeType;

//operations for numbers
typedef enum{
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, 
    OP_LESS, OP_LESS_EQ, OP_GR, OP_GR_EQ,
    OP_EQ, OP_NOT_EQ,
    OP_UNARY_MINUS, OP_UNARY_PLUS
} Operator;

struct Scope;

//structures for everything - union = only one structure at a time
typedef struct ASTNode{
    DataType data_type;
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
            struct Scope *scope;
            size_t count;
            size_t capacity;
        } block;

        struct {
            char *name;
            struct ASTNode **parameters;
            size_t param_count;
            size_t capacity;
            struct ASTNode *body;
            DataType return_type;
            bool has_explicit_return_type;
            bool is_main;
        } function_def;

        struct{
            char *name;
            size_t arg_count;
            size_t arg_capacity;
            struct ASTNode **arguments;
            bool is_builtin;
        } function_call;

        struct{
            char *name;
            bool var;    
            DataType type;                              
            struct ASTNode *value;
            size_t variable_id;
        } def_var;

        struct{
            Operator operation;
            struct ASTNode *left;
            struct ASTNode *right;
        } bin_op;

        struct{
            Operator operation;
            struct ASTNode *operand;
        } un_op;

        struct{
            struct ASTNode *condition;
            struct ASTNode *then_block;
            struct ASTNode *else_block;
        } if_stmt;

        struct{
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_loop;

        struct{
            char *name;
            struct ASTNode *value;
            size_t variable_id;
        } assign;

        struct{
            struct ASTNode *value;
        } return_stmt;

        struct{
            char *name;
            size_t variable_id;
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

        struct {
            char *name;
            DataType data;
            size_t variable_id;
        } param;

    } data;

} ASTNode;

ASTNode* create_node(NodeType type);
void free_ast(ASTNode *node);

void ast_array_append(ASTNode ***array, size_t *count, size_t *capacity, ASTNode *new_item);
 
#endif