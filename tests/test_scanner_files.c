//
// Created by Filip on 12. 10. 2026.
//


#include <stdio.h>

#include "../src/token.h"
#include "../src/scanner.h"

// compile (from root directory) by:
// gcc -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-implicit-fallthrough \
-Isrc \
tests/test_scanner_files.c \
src/scanner.c src/token.c src/dynamic_string.c src/keywords.c \
src/library.c src/sym_table.c \
-o tests/test_scanner_files

// run by:       tests/test_scanner_files



static const char *token_type_to_str(TokenType type) {
    switch (type) {
        case NO_TOKEN_FOUND: return "NO_TOKEN_FOUND";
        case T_IDENT:        return "T_IDENT";
        case T_NEWLINE:      return "T_NEWLINE";
        case T_AT_MAIN:      return "T_AT_MAIN";
        case T_INDENT:       return "T_INDENT";
        case T_INT:          return "T_INT";
        case T_DOUBLE:       return "T_DOUBLE";
        case T_STRING:       return "T_STRING";
        case T_PAR_L:        return "T_PAR_L";
        case T_PAR_R:        return "T_PAR_R";
        case T_SEMICOLON:    return "T_SEMICOLON";
        case T_COLON:        return "T_COLON";
        case T_COMMA:        return "T_COMMA";
        case T_DOT:          return "T_DOT";
        case T_KW_DEF:       return "T_KW_DEF";
        case T_KW_DO:        return "T_KW_DO";
        case T_KW_DOUBLE:    return "T_KW_DOUBLE";
        case T_KW_ELSE:      return "T_KW_ELSE";
        case T_KW_IF:        return "T_KW_IF";
        case T_KW_IMPORT:    return "T_KW_IMPORT";
        case T_KW_INT:       return "T_KW_INT";
        case T_KW_RETURN:    return "T_KW_RETURN";
        case T_KW_STRING:    return "T_KW_STRING";
        case T_KW_THEN:      return "T_KW_THEN";
        case T_KW_UNIT:      return "T_KW_UNIT";
        case T_KW_VAL:       return "T_KW_VAL";
        case T_KW_VAR:       return "T_KW_VAR";
        case T_KW_WHILE:     return "T_KW_WHILE";
        case T_KW_BOOLEAN:   return "T_KW_BOOLEAN";
        case T_KW_TRUE:      return "T_KW_TRUE";
        case T_KW_FALSE:     return "T_KW_FALSE";
        case T_KW_LIST:      return "T_KW_LIST";
        case T_OP_PLUS:      return "T_OP_PLUS";
        case T_OP_MINUS:     return "T_OP_MINUS";
        case T_OP_MUL:       return "T_OP_MUL";
        case T_OP_DIV:       return "T_OP_DIV";
        case T_OP_ASSIGN:    return "T_OP_ASSIGN";
        case T_OP_EQ:        return "T_OP_EQ";
        case T_OP_NOT_EQ:    return "T_OP_NOT_EQ";
        case T_OP_GR:        return "T_OP_GR";
        case T_OP_GR_EQ:     return "T_OP_GR_EQ";
        case T_OP_LESS:      return "T_OP_LESS";
        case T_OP_LESS_EQ:   return "T_OP_LESS_EQ";
        case T_OP_PLUS_PLUS: return "T_OP_PLUS_PLUS";
        case T_OP_ROCKET:    return "T_OP_ROCKET";
        case T_OP_BIN_NOT:   return "T_OP_BIN_NOT";
        case T_OP_BIN_AND:   return "T_OP_BIN_AND";
        case T_OP_BIN_OR:    return "T_OP_BIN_OR";
        case T_EOF:          return "T_EOF";
        default:             return "UNKNOWN_TOKEN";
    }
}



// USAGE: test_scanner_files {in_file} {out_file}
int main(int argc, char* argv[]) {
    if (argc != 3) {fprintf(stderr, "wrong arg count\n");return 1;}

    char* in_file = argv[1];
    char* out_file = argv[2];

    FILE* working_file = fopen(in_file, "r");
    FILE* result_file = fopen(out_file, "w");

    if (working_file == NULL || result_file == NULL) {
        fprintf(stderr, "TESTING ERROR: problem opening files\n");
    }

    Token token = {T_NEWLINE, NULL};
    while (token.type != T_EOF) {

        // get new token
        token = scanner(working_file, NULL, token.type);
        fputs(token_type_to_str(token.type), result_file);

        // print out indent level if applicable
        if (token.type == T_INDENT) {
            int spaces = token_get_indent_count(token);
            char str_spaces[32];
            snprintf(str_spaces, sizeof(str_spaces), "%d", spaces);

            fputs("(", result_file);
            fputs(str_spaces, result_file);
            fputs(")", result_file);
        }

        // if data is in token - print it out too
        char* data;
        if ((data = token_get_data_IDENT_INT_DOUBLE_STRING(token)) != NULL) {
            fputs("(", result_file);
            fputs(data, result_file);
            fputs(")", result_file);
        }

        if (token.type == T_NEWLINE) {
            fputs("\n", result_file);
        } else {
            fputs("   |   ", result_file);
        }
    }

    fclose(working_file);
    fclose(result_file);

    return 0;
}
