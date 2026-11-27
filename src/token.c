/**
 * @file token.c
 * @brief Implementation of TokenType and Token
 *
 * TokenType is implemented as an enum, while Token is a struct which
 * holds a TokenType and its data (string content, numerical value, ...)
 *
 * @author Filip Holan (xholanf00)
 * @date 4.10.2026
 */

#include "token.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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
        return -1;

    token->data = (void *)(intptr_t)indent_count;
    return 0;
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

bool token_free_data_if_applicable(Token token) {
    if ((token.type == T_IDENT || token.type == T_INT || token.type == T_DOUBLE ||
         token.type == T_STRING) &&
        token.data != NULL) {
        free(token.data);
        return true;
    }
    return false;
}
