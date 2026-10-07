//
// Created by Filip on 04/10/2026.
//


#include "sym_table.h"
#include "token.h"
#include "scanner.h"
#include "our_string.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>


#define VALID_CHARS_AFTER_NUMERAL "+-*/=!><&|();:,."  // todo not sure about this, especially the dot

bool is_digit(char c) {return '0' <= c && c <= '9';}

bool is_char_in_string(char* str, char c) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == c) {return true;}
    }
    return false;
}

void throw_lexical_error_1() {
    // todo lexical error 1
};

Token FSM_comments(FILE* file, SymTable *sym_table, TokenType last_token) {

}

Token FSM_numerals(FILE* file, SymTable *sym_table, TokenType last_token, char first_char) {
    typedef enum {
        F1, F2, F3, F4, F5, F6, F7, F8, F9
    } State;

    State state = first_char == '.' ? F3 : F1;  // if first char is ".", go straight to double entry point (F3), else F1
    String* result = str_init();
    int c;

    while ((c = fgetc(file)) != EOF) {
        switch (state) {

            // int loop
            case F1:
                if (is_digit(c)) {                              // digit looping
                    state = F1;
                    str_add(result, c);
                }
                else if (c == '_') {                            // _
                    state = F2;
                }
                else if (c == 'e' || c == 'E') {                // eE converts to double and proceeds to exponents
                    state = F6;
                    str_add(result, c);
                }
                else if (c == '.') {                            // . converts to double and proceeds to mantissa
                    state = F3;
                    str_add(result, c);
                }
                else {                                          // handle invalid last tokens, return result when valid
                    if ((first_char == '0' && result->len != 1) ||          // cannot start with 0 and be something else than 0
                        ('A' <= c && c <= 'z')) {                           // cannot be a letter (except {E,e} - handled above)
                        throw_lexical_error_1();
                    }
                    else {                                                  // return result
                        ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                        Token token = {INT, result};
                        return token;
                    }
                }
                break;

            // _ in INT
            case F2:
                if (is_digit(c)) {
                    state = F1;
                    str_add(result, c);
                } else {
                    throw_lexical_error_1();
                }
                break;

            // . in double
            case F3:
                if (is_digit(c)) {
                    state = F4;
                    str_add(result, c);
                } else {throw_lexical_error_1();}
                break;

            // double mantissa loop
            case F4:
                if (is_digit(c)) {
                    state = F4;
                    str_add(result, c);
                } else if (c == '_') {
                    state = F5;
                } else if (c == 'e' || c == 'E') {                // eE proceeds to exponents
                    state = F6;
                    str_add(result, c);
                } else if (c == 'd' || c == 'D') {                // dD proceeds to final decision state (F9)
                    state = F9;
                    str_add(result, c);
                } else {
                    if (c == '.' ||                     // if is a "." or a letter except "eEdD" throw error
                    ('A' <= c && c <= 'z')) {
                        throw_lexical_error_1();
                    } else {
                        ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                        Token token = {DOUBLE, result};
                        return token;
                    }
                }
                break;

            // _ for mantissa
            case F5:
                if (is_digit(c)) {
                    state = F4;
                    str_add(result, c);
                } else {
                    throw_lexical_error_1();
                }
                break;

            // exponent entry
            case F6:
                if (is_digit(c) || c == '+' || c == '-') {
                    state = F7;
                    str_add(result, c);
                } else {
                    throw_lexical_error_1();
                }
                break;

            // exponent loop
            case F7:
                if (is_digit(c)) {
                    state = F7;
                    str_add(result, c);
                } else if (c == '_') {
                    state = F8;
                } else if (c == 'd' || c == 'D') {                // dD proceeds to final decision state (F9)
                    state = F9;
                    str_add(result, c);
                } else {
                    if (!is_char_in_string(VALID_CHARS_AFTER_NUMERAL, c)) {
                        throw_lexical_error_1();
                    } else {
                        ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                        Token token = {DOUBLE, result};
                        return token;
                    }
                }
                break;

            // _ for exponent
            case F8:
                if (is_digit(c)) {
                    state = F7;
                    str_add(result, c);
                } else {
                    throw_lexical_error_1();
                }
                break;

            //
            case F9:
                if (!is_char_in_string(VALID_CHARS_AFTER_NUMERAL, c)) {
                    throw_lexical_error_1();
                } else {
                    ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                    Token token = {DOUBLE, result};
                    return token;
                }
                break;
        }
    }
    Token token = {T_EOF, NULL};
    return token;
}


Token scanner(FILE* file, SymTable *sym_table, TokenType last_token) {

    int c;
    while ((c = fgetc(file)) != EOF) {

        // numerals
        if (is_digit(c)) {
            return FSM_numerals(file, sym_table, last_token, c);
        }

        // @main
        if (c == '@') {
            char str[5] = "main";
            for (int i = 0; i < 4; i++) {
                if (fgetc(file) != str[i]) {
                    fprintf(stderr, "ERROR 1");
                    exit(1);
                }
            }
            Token token = {AT_MAIN, NULL};
            return token;
        }

        if (c == '/') {
            Token token;
            if ((token = FSM_comments(file, sym_table, last_token)).type == NO_TOKEN_FOUND) {
                continue;
            }
            return token;
        }
    }
    Token token = {T_EOF, NULL};
    return token;
}
