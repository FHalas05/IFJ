//
// Created by Filip on 04/10/2026.
//

#ifndef IFJ_SCANNER_H
#define IFJ_SCANNER_H

#include "sym_table.h"
#include "token.h"

Token scanner(FILE* file, SymTable sym_table, Token last_token);

#endif // IFJ_SCANNER_H
