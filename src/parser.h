
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "ast.h"
#include "sym_table.h"
#include "token.h"

typedef struct {
    FILE *src;
    SymTable *symtab;

    Token token;
    TokenType last_scanned;
}Parser;

void parse_program(FILE *file, SymTable *table);

#endif