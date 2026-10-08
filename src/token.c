//
// Created by Filip on 04/10/2026.
//

#include "token.h"


// returns the data stored in INDENT
// Incompatible Token type -> -1
int token_get_indent_count(Token token) {
    if (token.type == T_INDENT) {
        return *(int *) token.data;
    }
    return -1;
}


// returns the data stored in the {IDENT, INT, DOUBLE, STRING} types
// Incompatible Token type -> NULL
char *token_get_data_IDENT_INT_DOUBLE_STRING(Token token) {
    if (token.type == T_IDENT || token.type == T_INT || token.type == T_DOUBLE || token.type == T_STRING) {
        return (char *) token.data;
    }
    return NULL;
}