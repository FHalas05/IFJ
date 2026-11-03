
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
}Parser;

ASTNode *parse_program(FILE *file, SymTable *table);

void advance(Parser *p);

#endif