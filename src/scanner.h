//
// Created by Filip on 04/10/2026.
//

#ifndef IFJ_SCANNER_H
#define IFJ_SCANNER_H

#include <stdio.h>

#include "sym_table.h"
#include "token.h"

// Returns next token from file
// if file only contains EOF - will return EOF indefinitely without consuming it
Token scanner(FILE *file, SymTable *sym_table, TokenType last_token);

#endif // IFJ_SCANNER_H
