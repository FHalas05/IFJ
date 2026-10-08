//
// Created by Filip on 04/10/2026.
//


#include "sym_table.h"
#include "token.h"
#include "scanner.h"
#include "our_string.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WHITESPACE_CHARS " \t\n\v\f\r"

#define ENTRY_COMMENTS "/"
#define ENTRY_STRINGS  "\""
#define ENTRY_NUMERALS ".0123456789"
#define ENTRY_MAIN     "@"


bool string_contains(char* str, int c) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == c) {return true;}
    }
    return false;
}

void throw_lexical_error_1() {
    fprintf(stderr,"LEXICAL ERROR 1\n");
    exit(1);
}

/**
 * @brief FSM for comments and division
 * 
 * This FSM handels tokens for division ('/') , one line comments and block comments
 * 
 * @param file The programm soure file 
 * @param sym_table The table of symbols of compiler
 *
 * @return token tokens for '/' , one line comments and block commentsn
 */
Token FSM_comments(FILE* file, SymTable *sym_table) {
    typedef enum {
        F1, F2, F3, F4, F5, F6
    } State;

    State state = F1;

    int c;
    int nesting_depth = 0;
    bool new_line_check = false; // tracks whether there is a new line in block commnet
    while ((c = fgetc(file)) != EOF) {
        switch (state)
        {
        // division or comment
        case F1:
            if (c == '/') {
                state = F2;
            }
            else if (c == '*') {
                nesting_depth++;
                state = F3;
            }
            else {
                ungetc(c, file);
                Token token = {T_OP_DIV, NULL};
                return token;
            }
            break;
        // line comments
        case F2:
            if (c == '\n') {
                Token token = {T_NEWLINE, NULL};
                return token;
            }
            else {
                state = F2;
            }
            break;
        // innitial block commets
        case F3:
            if (c == '/'){
                state = F4;
            }
            else if (c == '*'){
                state = F5;
            }
            else if (c == '\n') {
                new_line_check = true;
            }
            else {
                state = F3;
            }
            break;
        //aditional nesting sequence
        case F4:
            if (c == '*') {
                state = F3;
                nesting_depth++;
            }
            else if (c == '/') {
                state = F4;
            }
            else if (c == '\n') {
                new_line_check = true;
                state = F3;
            }
            else {
                state = F3;
            }
            break;
        //aditional denesting sequence
        case F5:
            if (c == '/') {
                state = F3;
                nesting_depth--;
                if (nesting_depth == 0 && new_line_check) {
                    state = F6;
                }
                else if (nesting_depth == 0 && !new_line_check) {
                    Token token = {NO_TOKEN_FOUND};
                    return token;
                }
            }
            else if (c == '*') {
                state = F5;
            }
            else if (c == '\n') {
                new_line_check = true;
                state = F3;
            }
            else {
                state = F3;
            }
            break;
        // Checks to ensure that there is nothing but whitespace after a block comment followed by a newline
        case F6:
            if (c == ' ' || c == '\t' || c == '\r') {
                state = F6;
            }
            else if (c == '\n') {
                Token token = {T_NEWLINE, NULL};
                return token;
            }
            else {
                throw_lexical_error_1();
            }
            break;
        }
    }
    if (state == F1) { // handels case when division sign is last character of the file
        Token token = {T_OP_DIV, NULL};
        return token;
    }
    if (nesting_depth > 0) { // checks that no block commet was left open
        throw_lexical_error_1();
    }
    Token token = {T_EOF, NULL};
    return token;
}

/**
 * @brief FSM for strings
 * 
 * This FSM handels tokens for one-line string and multiple-line strings
 * 
 * @param file The programm soure file 
 * @param sym_table The symbols table of compiler
 * 
 * @return token tokens for one-line string or multiple-line strings
 */
Token FSM_string(FILE* file, SymTable *sym_table){
    typedef enum {
        F1, F2, F3, F4, F5, F6
    } State;

    State state = F1;
    String* final_string = str_init(); // string returned as token attribute
    int c;
    bool line_string = false; // check for multiple-line string

    while ((c = fgetc(file)) != EOF) {
        switch(state) {
            // initial division of string by type
            case F1:
                if (c == '\n') {
                    str_free(final_string);
                    throw_lexical_error_1();
                }
                else if (c == '"' && !line_string) { // possible multiple-line string or empty string
                    state = F2;
                }
                else if (c == '"' && line_string) { // final one line string
                    Token token = {T_STRING, final_string};
                    return token;
                }
                else if (c == '\\') { // check for presence of espcape sequences
                    line_string = true;
                    state = F6;
                }
                else { // one line string loop
                    line_string = true; 
                    str_add(final_string, c);
                    state = F1;
                }
                break;
            // secondary division of string by type    
            case F2:
                if (c == '"') {
                    state = F3;
                }
                else {
                    ungetc(c, file);
                    Token token = {T_STRING, final_string};
                    return token;
                }
                break;
            // possible ending sequence for multiple-line string or multiple-line string loop
            case F3:
                if (c == '"') {
                    state = F4;
                }
                else {
                    str_add(final_string, c);
                }
                break;
            // possible continuens of ending sequence for multiple-line string or " in string
            case F4:
                if (c == '"') {
                    state = F5;
                }
                else {
                    str_add(final_string, '"');
                    str_add(final_string, c);
                    state = F3;
                }
                break;
            // end of multiple-line string or "" in string
            case F5:
                if (c == '"') {
                    Token token = {T_STRING, final_string};
                    return token;
                }
                else {
                    str_add(final_string, '"');
                    str_add(final_string, '"');
                    str_add(final_string, c);
                    state = F3;
                }
                break;
            // handels escape squence recognition and insertion into string
            case F6:
                if (c == '\\') {
                    str_add(final_string, '\\');
                    state = F1;
                }
                else if (c == '"') {
                    str_add(final_string, '\"');
                    state = F1;
                }
                else if (c == 'n') {
                    str_add(final_string, '\n');
                    state = F1;
                }
                else if (c == 'r') {
                    str_add(final_string, '\r');
                    state = F1;
                }
                else if (c == 't') {
                    str_add(final_string, '\t');
                    state = F1;
                }
                else {
                    str_free(final_string);
                    throw_lexical_error_1();
                }
                break;
        }
    }
    str_free(final_string);
    throw_lexical_error_1();
}

Token FSM_numerals(FILE* file, SymTable *sym_table, int first_char) {
    typedef enum {
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10
    } State;

    State state = first_char == '.' ? F3 : F1;  // if first char is ".", go straight to double entry point (F3), else F1
    String* result = str_init();
    str_add(result, first_char);
    int c;

    while ((c = fgetc(file)) != EOF) {
        switch (state) {

            // int loop
            case F1:
                if (isdigit(c)) {  // digit looping
                    state = F1;
                    str_add(result, c);
                }
                else if (c == '_') {  // _
                    state = F2;
                }
                else if (c == 'e' || c == 'E') {  // eE converts to double and proceeds to exponents
                    state = F6;
                    str_add(result, c);
                }
                else if (c == '.') {  // . converts to double and proceeds to mantissa
                    state = F3;
                    str_add(result, c);
                }
                else {                                          // handle invalid last tokens, return result when valid
                    if ((first_char == '0' && result->len != 1) ||          // cannot start with 0 and be something else than 0
                        isalpha(c)) {                           // cannot be a letter (except {E,e} - handled above)
                        throw_lexical_error_1();
                    }
                    else {                                                  // return result
                        ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                        Token token = {T_INT, result};
                        return token;
                    }
                }
                break;

            // _ in INT
            case F2:
                if (isdigit(c)) {
                    state = F1;
                    str_add(result, c);
                } else { // there must always be a digit after '_'
                    throw_lexical_error_1();
                }
                break;

            // . in double / double entry point
            case F3:
                if (isdigit(c)) {  // process to the mantissa loop
                    state = F4;
                    str_add(result, c);
                }
                else {  // if the first char was '.' and theres no number after it, then it must be the dot operator
                    if (first_char == '.') {
                        ungetc(c, file);
                        Token token = {T_DOT, result};
                        return token;
                    }
                    throw_lexical_error_1();  // if its something else than '.' or a numeral, its invalid
                }
                break;

            // double mantissa loop
            case F4:
                if (isdigit(c)) {
                    state = F4;
                    str_add(result, c);
                }
                else if (c == '_') {
                    state = F5;
                }
                else if (c == 'e' || c == 'E') {  // eE proceeds to exponents
                    state = F6;
                    str_add(result, c);
                }
                else if (c == 'd' || c == 'D') {  // dD proceeds to final decision state (F10)
                    state = F10;
                    str_add(result, c);
                }
                else {
                    if (c == '.' || isalpha(c)) {  // if is a "." or a letter except "eEdD" throw error
                        throw_lexical_error_1();
                    }
                    else {
                        ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                        Token token = {T_DOUBLE, result};
                        return token;
                    }
                }
                break;

            // _ for mantissa
            case F5:
                if (isdigit(c)) {
                    state = F4;
                    str_add(result, c);
                }
                else {
                    throw_lexical_error_1();
                }
                break;

            // exponent entry
            case F6:
                if (isdigit(c)) {
                    state = F8;
                    str_add(result, c);
                }
                else if (c == '+' || c == '-') {
                    state = F7;
                    str_add(result, c);
                }
                else {
                    throw_lexical_error_1();
                }
                break;

            // exponent sign
            case F7:
                if (isdigit(c)) {
                    state = F8;
                    str_add(result, c);
                }
                else {
                    throw_lexical_error_1();
                }
                break;

            // exponent loop
            case F8:
                if (isdigit(c)) {
                    state = F8;
                    str_add(result, c);
                }
                else if (c == '_') {
                    state = F9;
                }
                else if (c == 'd' || c == 'D') {                // dD proceeds to final decision state (F10)
                    state = F10;
                    str_add(result, c);
                }
                else {
                    if (isalpha(c)) {  // other letters than dD throw error
                        throw_lexical_error_1();
                    }
                    else {
                        ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                        Token token = {T_DOUBLE, result};
                        return token;
                    }
                }
                break;

            // _ for exponent
            case F9:
                if (isdigit(c)) {
                    state = F8;
                    str_add(result, c);
                }
                else {
                    throw_lexical_error_1();
                }
                break;

            // final decision state
            case F10:
                if (isalpha(c)) {  // other letters than dD throw error
                    throw_lexical_error_1();
                }
                else {
                    ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                    Token token = {T_DOUBLE, result};
                    return token;
                }
                break;
        }
    }
    Token token = {T_EOF, NULL};
    return token;
}

Token HANDLER_main(FILE* file, SymTable *sym_table) {
    // compare to "main"
    char str[5] = "main";
    for (int i = 0; i < 4; i++) {
        if (fgetc(file) != str[i]) {
            throw_lexical_error_1();
        }
    }

    // make sure its followed by whitespace
    int c;
    if (string_contains(WHITESPACE_CHARS, c = fgetc(file))) {
        ungetc(c, file);
        Token token = {T_AT_MAIN, NULL};
        return token;
    }
    throw_lexical_error_1();
}

Token scanner(FILE* file, SymTable *sym_table, TokenType last_token) {

    int c;
    while ((c = fgetc(file)) != EOF) {

        if (string_contains(ENTRY_COMMENTS, c)) {
            Token token;
            if ((token = FSM_comments(file, sym_table)).type == NO_TOKEN_FOUND)
                continue;
            return token;
        }

        if (string_contains(ENTRY_STRINGS, c))
            return FSM_string(file, sym_table);

        if (string_contains(ENTRY_NUMERALS, c))
            return FSM_numerals(file, sym_table, c);

        if (string_contains(ENTRY_MAIN, c))
            return HANDLER_main(file, sym_table);


    }
    Token token = {T_EOF, NULL};
    return token;
}
