
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "ast.h"
#include "symtable.h"
#include "token.h"

typedef struct {
    FILE *src;
    SymTable *symtab;

    Token token;
    TokenType last_scanned;

    Token peek_token;
    bool has_peek;
}Parser;

ASTNode *parse_program(FILE *file, SymTable *table);

void advance(Parser *p);
void expect(Parser *p, TokenType type, char *msg);
TokenType peek_token_type(Parser *p);

#endif