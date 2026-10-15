//
// Created by Filip on 04/10/2026.
//

#include "token.h"

#include <stddef.h>
#include <stdint.h>

// returns the indent count stored in a INDENT token
// Incompatible Token type -> -1
int token_get_indent_count(Token token) {
    // using allocated space for int, instead of the pointer it was allocated for
    // to assign a value to this, use:    token.data = (void*)(intptr_t)spaces

    // (the casting to intptr_t is to suppress compiler warnings,
    // it is always exactly the size of pointer no matter the architecture)

    if (token.type == T_INDENT)
        return (int)(intptr_t)token.data;
    return -1;
}

// stores the indent level in a INDENT type token
// invalid token type -> -1, else 0
int token_set_indent_count(Token *token, int indent_count) {
    if (token->type != T_INDENT)
        return 0;

    token->data = (void *)(intptr_t)indent_count;
    return -1;
}

// returns the data stored in the {IDENT, INT, DOUBLE, STRING} types
// Incompatible Token type -> NULL
char *token_get_data_IDENT_INT_DOUBLE_STRING(Token token) {
    if (token.type == T_IDENT || token.type == T_INT || token.type == T_DOUBLE ||
        token.type == T_STRING) {
        return (char *)token.data;
    }
    return NULL;
}
