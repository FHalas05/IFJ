//
// Created by Filip on 04/10/2026.
//

#ifndef IFJ_TOKEN_H
#define IFJ_TOKEN_H


#include <stddef.h>

typedef enum {
    IDENT,
    INDENT,
    AT_MAIN,
    INT,
    DOUBLE,
    STRING
    // all tokens go here
} TokenType;


typedef struct {
    TokenType type;;
    void *data;
} Token;


// returns the data stored in INDENT
// Incompatible Token type -> -1
int token_get_indent_count(Token token) {
    if (token.type == INDENT) {
        return *(int *) token.data;
    }
    return -1;
}


// returns the data stored in the {IDENT, INT, DOUBLE, STRING} types
// Incompatible Token type -> NULL
char *token_get_data_IDENT_INT_DOUBLE_STRING(Token token) {
    if (token.type == IDENT || token.type == INT || token.type == DOUBLE || token.type == STRING) {
        return (char *) token.data;
    }
    return NULL;
}

#endif // IFJ_TOKEN_H
