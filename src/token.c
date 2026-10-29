//
// Created by Filip on 04/10/2026.
//

#include "token.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


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
       token.type == T_STRING) && token.data != NULL) {
        free(token.data);
        return true;
    }
    return false;
}
