//
// Created by Filip on 04/10/2026.
//

#ifndef IFJ_SCANNER_H
#define IFJ_SCANNER_H

#include <stdio.h>

#include "sym_table.h"
#include "token.h"

/**
 * @brief Returns next token from file
 *
 * Scans file and returns the next token or throws error if no token type matches
 *
 * @warning first run should use T_NEWLINE as last_token_type
 *
 * @param file The program source file
 * @param sym_table The table of symbols of compiler
 * @param last_token_type The type of the last token this function has returned (The initial one should be T_NEWLINE)
 *
 * @return Matched token
 */
Token scanner(FILE *file, SymTable *sym_table, TokenType last_token_type);

#endif // IFJ_SCANNER_H
