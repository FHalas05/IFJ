/**
 * @file parser.h
 * @brief Parser interface
 *
 * Defines the parser state and the functions used by other parts of the compiler.
 *
 * @author Jakub Juřík (xjurikj00)
 * @date 4.10.2026
 */

#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdio.h>

#include "ast.h"
#include "symtable.h"
#include "token.h"

/**
 * @brief State of the parser
 */
typedef struct {
    FILE *src;        // input file
    SymTable *symtab; // symbol table
    ASTNode *program; // program node; lambdas are added to its function list

    Token token; // current token

    Token peek_token; // next token ready by peek_token_type
    bool has_peek;    // true if peek_token is valid

    Token pending_token;    // token saved for later by next_token
    bool has_pending_token; // true if pending_token is valid

    int *indent_stack;      // stack of indentation levels (spaces)
    size_t indent_count;    // number of levels on the stack
    size_t indent_capacity; // size of the stack

    size_t pending_dedent; // number of DEDENT tokens still to return
    bool line_start;       // true at the start of a new line
    bool eof_newline_done; // true when the NEWLINE before EOF was added

    size_t paren_depth;    // how deep we are inside bracket
    int logical_indent;    // indentation of the current statement
    bool pending_lf;       // true if there was a new line inside brackets
    bool ended_with_block; // true if the last expression was a lambda with a block body (no NEWLINE
                           // follows)
} Parser;

/**
 * @brief Parses a whole program and returns its AST.
 *
 * Rule 1: PROGRAM -> PROLOG FUNCTION_LIST
 *
 * @param file Input file
 * @param table Symbol table
 *
 * @return Root of the AST (NODE_PROGRAM)
 */
ASTNode *parse_program(FILE *file, SymTable *table);

/**
 * @brief Parses the parameters of a function or a lambda.
 *
 * Rule 17: PARAMS -> ONE_PARAM PARAM_TAIL
 * Rule 18: PARAMS -> ε
 *
 * @param p Parser
 * @param func Function node
 */
void parse_params(Parser *p, ASTNode *func);

/**
 * @brief Parses an indented block.
 *
 * Used for function bodies, if/while bodies and lambda bodies.
 *
 * Rule 26: BLOCK -> T_NEWLINE T_INDENT STMT_LIST
 *
 * @param p Parser
 *
 * @return Block node (NODE_BLOCK)
 */
ASTNode *parse_block(Parser *p);

/**
 * @brief Moves to the next token
 *
 * Frees the data of the current token if the AST did not take it.
 *
 * @param p Parser
 */
void advance(Parser *p);

/**
 * @brief Checks the current token and moves to the next one.
 *
 * If the token is wrong, ends with syntax error 2.
 *
 * @param p Parser
 * @param type Expected token type
 * @param msg Error message
 */
void expect(Parser *p, TokenType type, char *msg);

/**
 * @brief Looks at the next token, but does not move it.
 *
 * Used to know if "id" starts an assignment (next token is '=') or an expression (rule 34 and 36).
 *
 * @param p Parser
 *
 * @return Type of the next token
 */
TokenType peek_token_type(Parser *p);

#endif
