
#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdio.h>

#include "ast.h"
#include "symtable.h"
#include "token.h"

typedef struct {
    FILE *src;
    SymTable *symtab;

    Token token;

    Token peek_token;
    bool has_peek;

    Token pending_token;
    bool has_pending_token;

    int *indent_stack;
    size_t indent_count;
    size_t indent_capacity;

    size_t  pending_dedent;
    bool line_start;
    bool eof_newline_done;

    size_t paren_depth;
    size_t logical_indent;
    bool pending_lf;
}Parser;

ASTNode *parse_program(FILE *file, SymTable *table);

void advance(Parser *p);
void expect(Parser *p, TokenType type, char *msg);
TokenType peek_token_type(Parser *p);

#endif