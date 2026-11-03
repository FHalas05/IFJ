//
// Created by Filip Halaš on 31/10/2026.
//

#ifndef IFJ_EXPRESSION_PARSER_H
#define IFJ_EXPRESSION_PARSER_H

#include "parser.h"

typedef enum {
    P_ADD,          // +, -
    P_MUL,          // *, /
    P_REL,          // <, >, <=, >=
    P_EQ,           // ==, !=
    P_LPAREN,       // (
    P_RPAREN,       // )
    P_UNARY_MINUS,  // ~ (unary minus)
    P_OPERAND,      // i
    P_END,          // $
} PrecTerminal;

void parse_expression(Parser *parser);

#endif // IFJ_EXPRESSION_PARSER_H
