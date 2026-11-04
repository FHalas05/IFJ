//
// Created by Filip on 04/10/2026.
//

#ifndef IFJ_SCANNER_H
#define IFJ_SCANNER_H

#include <stdio.h>

#include "token.h"

/**
 * @brief Returns next token from file
 *
 * Scans file and returns the next token or throws error if no token type matches
 *
 * @warning first run should use T_NEWLINE as last_token_type
 * @warning you MUST free the data in tokens which include additional data in string form
 * i.e. free(token.data)
 *
 * @param file The program source file
 *
 * @return Matched token
 */
Token scanner(FILE *file);

#endif // IFJ_SCANNER_H
