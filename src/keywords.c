//
// Created by Šimon on 9/10/2026
//

#include "keywords.h"

#include <stdio.h>
#include <string.h>


typedef struct {
    const char* keyword;
    TokenType type;
} KeyWordItem;

static const KeyWordItem keywords[] = {
    {"def", T_KW_DEF},
    {"do", T_KW_DO},
    {"Double", T_KW_DOUBLE},
    {"else", T_KW_ELSE},
    {"if", T_KW_IF},
    {"import", T_KW_IMPORT},
    {"Int", T_KW_INT},
    {"return", T_KW_RETURN},
    {"String", T_KW_STRING},
    {"then", T_KW_THEN},
    {"Unit", T_KW_UNIT},
    {"val", T_KW_VAL},
    {"var", T_KW_VAR},
    {"while", T_KW_WHILE},
    {"Boolean", T_KW_BOOLEAN},
    {"true", T_KW_TRUE},
    {"false", T_KW_FALSE},
    {"List", T_KW_LIST},
};

#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

Token is_keyword(DynamicString* string) {
    if (!strcmp(string->string, "_")) {
            Token token = {NO_TOKEN_FOUND, NULL};
            dyn_str_free(string);
            return token;
    }
    for (size_t i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(string->string, keywords[i].keyword) == 0) {
            Token token = {keywords[i].type, NULL};
            dyn_str_free(string);
            return token;
        }
    }
    Token token = {T_IDENT, string->string};
    return token;
}