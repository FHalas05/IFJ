
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "ast.h"
#include "sym_table.h"

void parse_program(FILE *file, SymTable *table);

#endif