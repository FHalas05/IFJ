/**
 * @file ast.h
 * @brief Abstract syntax tree (AST) of IFJ26
 *
 * The parser builds the AST while it reads the program.
 * The semantic analysis checks it and fills in the types (data_type).
 * The code generator goes through it and makes IFJcode26.
 *
 * Every node has a type (NodeType) and data in a union.
 * Only the part of the union that belongs to the node type is used.
 *
 * @author Jakub Juřík (xjurikj00)
 * @date 6.10.2026
 */

#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <stdbool.h>

/**
 * @brief Types of values in IFJ26
 */
typedef enum{
    TYPE_UNKNOWN,       // Type is not known yet (not written, found by the semantic analysis)
    TYPE_ANY,           // Any type (only for parameters of print and toStr)
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_BOOL
} DataType;

/**
 * @brief Kinds of nodes in the AST
 */
typedef enum{
    NODE_PROGRAM,                // Whole program (list of functions)
    NODE_BLOCK,                  // Block of statements
    NODE_FUNCTION_DEF,           // Function definition
    NODE_FUNCTION_CALL,          // Function call
    NODE_DEF_VAR,                // Variable definition (val, var)
    NODE_BINOP,                  // Binary operation (a + b)
    NODE_UNOP,                   // Unary operation (-a, !a)
    NODE_IF,                     // if statement
    NODE_WHILE,                  // while loop
    NODE_ASSIGN,                 // Assignment (x = 1)
    NODE_RETURN,                 // return
    NODE_IDENTIFIER,             // Variable name in an expression
    NODE_INT_LITERAL,            // Int value
    NODE_DOUBLE_LITERAL,         // Double value
    NODE_STRING_LITERAL,         // String value
    NODE_BOOL_LITERAL,           // true / false
    NODE_UNIT_LITERAL,           // ()
    NODE_PARAM                   // Function parameter
} NodeType;

/**
 * @brief Operators used in NODE_BINOP and NODE_UNOP
 */
typedef enum{
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,             // + - * /
    OP_LESS, OP_LESS_EQ, OP_GR, OP_GR_EQ,       // < <= > >=
    OP_EQ, OP_NOT_EQ,                           // == !=
    OP_UNARY_MINUS, OP_NOT, OP_AND, OP_OR,      // -x  !x  &&  ||
} Operator;

struct Scope;

/**
 * @brief One node of the AST
 */
typedef struct ASTNode{
    DataType data_type;     // Type of the value (set by the semantic analysis for expressions)
    NodeType type;          // Kind of the node (says which part of the union is used)
    bool widen_to_double;   // Convert the expression result from Int to Double
    int line;               // Line in the source code
    int column;             // Column in the source code

    // Data of the node; only the part for this node type is used
    union{
        // NODE_PROGRAM
        struct{
            struct ASTNode **functions;     // Array of functions (NODE_FUNCTION_DEF)
            size_t count;                   // Number of functions
            size_t capacity;                // Size of the array
        } program;

        // NODE_BLOCK
        struct {
            struct ASTNode **statements;    // Array of statements
            struct Scope *scope;            // Scope of the block
            size_t count;                   // Number of statements
            size_t capacity;                // Size of the array
        } block;

        // NODE_FUNCTION_DEF
        struct {
            char *name;                         // Function name
            struct ASTNode **parameters;        // Array of parameters (NODE_PARAM)
            size_t param_count;                 // Number of parameters
            size_t capacity;                    // Size of the array
            struct ASTNode *body;               // Body (always NODE_BLOCK, also a one-line body)
            DataType return_type;               // Return type (written or found by the semantic analysis)
            bool has_explicit_return_type;      // true if the return type is written in the code
            bool is_main;                       // true if the function has @main
            int check_state;                    // Semantic analysis: 0 = not checked, 1 = being checked, 2 = done
            struct ASTNode *caller;             // Semantic analysis: function that started the check (for recursion)
        } function_def;

        // NODE_FUNCTION_CALL
        struct{
            char *name;                         // Name of the called function
            size_t arg_count;                   // Number of arguments
            size_t arg_capacity;                // Size of the array
            struct ASTNode **arguments;         // Array of arguments (expressions)
            bool is_builtin;                    // true for built-in functions (print, readInt, ...)
        } function_call;

        // NODE_DEF_VAR
        struct{
            char *name;                         // Variable name
            bool var;                           // true for var (can be changed), false for val
            DataType type;                      // Written type, or TYPE_UNKNOWN if it is not written
            struct ASTNode *value;              // Value (expression)
            size_t variable_id;                 // Unique number of the variable
        } def_var;

        // NODE_BINOP
        struct{
            Operator operation;                 // Operator
            struct ASTNode *left;               // Left side
            struct ASTNode *right;              // Right side
        } bin_op;

        // NODE_UNOP
        struct{
            Operator operation;                 // OP_UNARY_MINUS or OP_NOT
            struct ASTNode *operand;            // Value the operator is used on
        } un_op;

        // NODE_IF
        struct{
            struct ASTNode *condition;          // Condition (Boolean expression)
            struct ASTNode *then_block;         // Block when the condition is true
            struct ASTNode *else_block;         // Block when it is false; NULL if there is no else.
                                                // For "else if" it is a block with one NODE_IF.
        } if_stmt;

        // NODE_WHILE
        struct{
            struct ASTNode *condition;          // Condition (Boolean expression)
            struct ASTNode *body;               // Body of the loop (NODE_BLOCK)
        } while_loop;

        // NODE_ASSIGN
        struct{
            char *name;                         // Variable name
            struct ASTNode *value;              // New value (expression)
            size_t variable_id;                 // Unique number of the variable
        } assign;

        // NODE_RETURN
        struct{
            struct ASTNode *value;              // Returned value; NULL for "return" without a value
        } return_stmt;

        // NODE_IDENTIFIER
        struct{
            char *name;                         // Variable name
            size_t variable_id;                 // Unique number of the variable
        } identifier;

        // NODE_INT_LITERAL
        struct{
            long value;
        } int_lit;

        // NODE_DOUBLE_LITERAL
        struct{
            double value;
        } double_lit;

        // NODE_STRING_LITERAL
        struct{
            char *value;
        } string_lit;

        // NODE_BOOL_LITERAL
        struct {
            bool value;
        } bool_lit;

        // NODE_PARAM
        struct {
            char *name;                         // Parameter name
            DataType data;                      // Parameter type
            size_t variable_id;                 // Unique number of the parameter
        } param;

    } data;

} ASTNode;


/**
 * @brief Makes a new empty node.
 *
 * All data is set to 0 / NULL and data_type to TYPE_UNKNOWN.
 * Ends the program with code 99 if there is no memory.
 *
 * @param type Kind of the node
 * @return New node
 */
ASTNode* create_node(NodeType type);


/**
 * @brief Frees a node and all its children (the whole subtree).
 *
 * @param node Node to free (can be NULL)
 */
void free_ast(ASTNode *node);


/**
 * @brief Adds a node to the end of a dynamic array.
 *
 * The array gets bigger (2x) when it is full.
 *
 * @param array Pointer to the array
 * @param count Pointer to the number of items
 * @param capacity Pointer to the size of the array
 * @param new_item Node to add
 */
void ast_array_append(ASTNode ***array, size_t *count, size_t *capacity, ASTNode *new_item);

#endif