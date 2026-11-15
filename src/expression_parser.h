/**
 * @file expression_parser.h
 * @brief Expression parser interface
 *
 * Defines expression parsing modes and precedence categories used
 * to build AST nodes from expressions.
 *
 * @author Filip Halaš (xhalasf00)
 * @date 31.10.2026
 */

#ifndef IFJ_EXPRESSION_PARSER_H
#define IFJ_EXPRESSION_PARSER_H

#include "parser.h"

/**
 * @brief Parsing mode that determines where an expression ends
 */
typedef enum {
    EXPR_STATEMENT, // Expression ending at a statement or condition delimiter
    EXPR_ARGUMENT,  // Argument ending at a comma or closing parenthesis outside nested parentheses
} ExprMode;

/**
 * @brief Terminal categories used in precedence comparisons
 */
typedef enum {
    P_ADD,          // +, -
    P_MUL,          // *, /
    P_REL,          // <, >, <=, >=
    P_EQ,           // ==, !=
    P_AND,          // &&
    P_OR,           // ||
    P_LPAREN,       // (
    P_RPAREN,       // )
    P_UNARY,        // ~ (unary minus), !
    P_OPERAND,      // i (literal, identifier or function call)
    P_END,          // $ (end of the expression)
} PrecTerminal;

/**
 * @brief Parses an expression and builds its AST
 *
 * Uses operator precedence and handles parentheses, unary operators
 * and function calls. Leaves the token ending the expression
 * in parser->token for the caller.
 *
 * @param parser Parser positioned at the first expression token; must not be NULL
 * @param mode Determines which tokens can end the expression
 *
 * @return Pointer to the root AST node of the parsed expression
 */
ASTNode *parse_expression(Parser *parser, ExprMode mode);

#endif // IFJ_EXPRESSION_PARSER_H
