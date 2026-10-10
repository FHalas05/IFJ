//
// Created by Filip on 04/10/2026.
//


#include "sym_table.h"
#include "token.h"
#include "scanner.h"
#include "dynamic_string.h"
#include "keywords.h"
#include "library.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define WHITESPACE_CHARS " \t\n\v\f\r"

#define ENTRY_COMMENTS "/"
#define ENTRY_STRINGS  "\""
#define ENTRY_NUMERALS ".0123456789"
#define ENTRY_MAIN     "@"
#define ENTRY_OPERATORS "=!><+-*&|"
#define ENTRY_SPECIAL  ";,:()\n"
#define ENTRY_WHITESPACE  " \t\v\f\r"
#define ENTRY_IDENTIFIERS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYXZ_"

#define throw_lexical_error(description) throw_fatal_error(1, description)

/**
 * @brief FSM for comments and division
 *
 * This FSM handles tokens for division ('/') , one line comments and block comments
 *
 * @param file The program source file
 * @param sym_table The table of symbols of compiler
 *
 * @return token tokens for '/' , one line comments and block comments
 */
Token FSM_comments(FILE *file, SymTable *sym_table) {
    typedef enum {
        F1, F2, F3, F4, F5, F6
    } State;

    State state = F1;

    int c;
    int nesting_depth = 0;
    bool new_line_check = false; // tracks whether there is a new line in block comment
    while ((c = fgetc(file)) != EOF) {
        switch (state) {
            // division or comment
            case F1:
                if (c == '/') {
                    state = F2;
                } else if (c == '*') {
                    nesting_depth++;
                    state = F3;
                } else {
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
                } else {
                    state = F2;
                }
                break;
            // initial block comments
            case F3:
                if (c == '/') {
                    state = F4;
                } else if (c == '*') {
                    state = F5;
                } else if (c == '\n') {
                    new_line_check = true;
                } else {
                    state = F3;
                }
                break;
            //additional nesting sequence
            case F4:
                if (c == '*') {
                    state = F3;
                    nesting_depth++;
                } else if (c == '/') {
                    state = F4;
                } else if (c == '\n') {
                    new_line_check = true;
                    state = F3;
                } else {
                    state = F3;
                }
                break;
            //additional denesting sequence
            case F5:
                if (c == '/') {
                    state = F3;
                    nesting_depth--;
                    if (nesting_depth == 0 && new_line_check) {
                        state = F6;
                    } else if (nesting_depth == 0 && !new_line_check) {
                        Token token = {NO_TOKEN_FOUND};
                        return token;
                    }
                } else if (c == '*') {
                    state = F5;
                } else if (c == '\n') {
                    new_line_check = true;
                    state = F3;
                } else {
                    state = F3;
                }
                break;
            // Checks to ensure that there is nothing but whitespace after a block comment followed by a newline
            case F6:
                if (c == ' ' || c == '\t' || c == '\r') {
                    state = F6;
                } else if (c == '\n') {
                    Token token = {T_NEWLINE, NULL};
                    return token;
                } else {
                    throw_lexical_error("FSM_comments - F6");
                }
                break;
        }
    }
    if (state == F1) {
        // handles case when division sign is last character of the file
        Token token = {T_OP_DIV, NULL};
        return token;
    }
    if (nesting_depth > 0) {
        // checks that no block comment was left open
        throw_lexical_error("FSM_comments - multiline comment was left open");
    }
    Token token = {T_EOF, NULL};
    return token;
}

/**
 * @brief FSM for strings
 *
 * This FSM handles tokens for one-line string and multiple-line strings
 *
 * @param file The program source file
 * @param sym_table The symbols table of compiler
 *
 * @return token tokens for one-line string or multiple-line strings
 */
Token FSM_strings(FILE *file, SymTable *sym_table) {
    typedef enum {
        F1, F2, F3, F4, F5, F6
    } State;

    State state = F1;
    DynamicString *final_string = dyn_str_init(); // string returned as token attribute
    int c;
    bool line_string = false; // check for multiple-line string

    while ((c = fgetc(file)) != EOF) {
        switch (state) {
            // initial division of string by type
            case F1:
                if (c == '\n') {
                    dyn_str_free(final_string);
                    throw_lexical_error("FSM_strings - F1");
                } else if (c == '"' && !line_string) {
                    // possible multiple-line string or empty string
                    state = F2;
                } else if (c == '"' && line_string) {
                    // final one line string
                    Token token = {T_STRING, final_string};
                    return token;
                } else if (c == '\\') {
                    // check for presence of escape sequences
                    line_string = true;
                    state = F6;
                } else {
                    // one line string loop
                    line_string = true;
                    dyn_str_add(final_string, c);
                    state = F1;
                }
                break;
            // secondary division of string by type
            case F2:
                if (c == '"') {
                    state = F3;
                } else {
                    ungetc(c, file);
                    Token token = {T_STRING, final_string};
                    return token;
                }
                break;
            // possible ending sequence for multiple-line string or multiple-line string loop
            case F3:
                if (c == '"') {
                    state = F4;
                } else {
                    dyn_str_add(final_string, c);
                }
                break;
            // possible continues of ending sequence for multiple-line string or " in string
            case F4:
                if (c == '"') {
                    state = F5;
                } else {
                    dyn_str_add(final_string, '"');
                    dyn_str_add(final_string, c);
                    state = F3;
                }
                break;
            // end of multiple-line string or "" in string
            case F5:
                if (c == '"') {
                    Token token = {T_STRING, final_string};
                    return token;
                } else {
                    dyn_str_add(final_string, '"');
                    dyn_str_add(final_string, '"');
                    dyn_str_add(final_string, c);
                    state = F3;
                }
                break;
            // handles escape sequence recognition and insertion into string
            case F6:
                if (c == '\\') {
                    dyn_str_add(final_string, '\\');
                    state = F1;
                } else if (c == '"') {
                    dyn_str_add(final_string, '\"');
                    state = F1;
                } else if (c == 'n') {
                    dyn_str_add(final_string, '\n');
                    state = F1;
                } else if (c == 'r') {
                    dyn_str_add(final_string, '\r');
                    state = F1;
                } else if (c == 't') {
                    dyn_str_add(final_string, '\t');
                    state = F1;
                } else {
                    dyn_str_free(final_string);
                    throw_lexical_error("FSM_strings - F6");
                }
                break;
        }
    }
    dyn_str_free(final_string); // todo figure out whether to do this or not
    throw_lexical_error("FSM_strings - end state");
}

/**
 * @brief FSM for numerals (int, double)
 *
 * This FSM handles all numeral types and the dot operator
 *
 * @param file The program source file
 * @param sym_table The table of symbols of compiler
 *
 * @return token token of numeral with its content
 */
Token FSM_numerals(FILE *file, SymTable *sym_table, int first_char) {
    typedef enum {
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10
    } State;

    State state = first_char == '.' ? F3 : F1; // if first char is ".", go straight to double entry point (F3), else F1
    DynamicString *result = dyn_str_init();
    dyn_str_add(result, first_char);
    int c;

    while (42) {
        c = fgetc(file);
        switch (state) {
            // int loop
            case F1:
                if (isdigit(c)) {
                    // digit looping
                    state = F1;
                    dyn_str_add(result, c);
                } else if (c == '_') {
                    // _
                    state = F2;
                } else if (c == 'e' || c == 'E') {
                    // eE converts to double and proceeds to exponents
                    state = F6;
                    dyn_str_add(result, c);
                } else if (c == '.') {
                    // . converts to double and proceeds to mantissa
                    state = F3;
                    dyn_str_add(result, c);
                } else {
                    // handle invalid last tokens, return result when valid
                    if ((first_char == '0' && result->len != 1) || // cannot start with 0 and be something else than 0
                        isalpha(c)) {
                        // cannot be a letter (except {E,e} - handled above)
                        throw_lexical_error("FSM_numerals - Nonzero number starts with 0, OR an invalid letter follows the number");
                    } else {
                        // return result
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
                    dyn_str_add(result, c);
                } else {
                    // there must always be a digit after '_'
                    throw_lexical_error("FSM_numerals - F2 - No digit after '_'");
                }
                break;

            // . in double / double entry point
            case F3:
                if (isdigit(c)) {
                    // process to the mantissa loop
                    state = F4;
                    dyn_str_add(result, c);
                } else {
                    // if the first char was '.' and theres no number after it, then it must be the dot operator
                    if (first_char == '.') {
                        ungetc(c, file);
                        Token token = {T_DOT, result};
                        return token;
                    }
                    throw_lexical_error("FSM_numerals - F3 - there is no digit after a decimal point"); // if its something else than '.' or a numeral, its invalid
                }
                break;

            // double mantissa loop
            case F4:
                if (isdigit(c)) {
                    state = F4;
                    dyn_str_add(result, c);
                } else if (c == '_') {
                    state = F5;
                } else if (c == 'e' || c == 'E') {
                    // eE proceeds to exponents
                    state = F6;
                    dyn_str_add(result, c);
                } else if (c == 'd' || c == 'D') {
                    // dD proceeds to final decision state (F10)
                    state = F10;
                    dyn_str_add(result, c);
                } else {
                    if (c == '.' || isalpha(c)) {
                        // if is a "." or a letter except "eEdD" throw error
                        throw_lexical_error("FSM_numerals - F4 - Invalid char after number");
                    } else {
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
                    dyn_str_add(result, c);
                } else {
                    throw_lexical_error("FSM_numerals - F5 - No digit after '_'");
                }
                break;

            // exponent entry
            case F6:
                if (isdigit(c)) {
                    state = F8;
                    dyn_str_add(result, c);
                } else if (c == '+' || c == '-') {
                    state = F7;
                    dyn_str_add(result, c);
                } else {
                    throw_lexical_error("FSM_numerals - F6 - Invalid char after exponent (e/E)");
                }
                break;

            // exponent sign
            case F7:
                if (isdigit(c)) {
                    state = F8;
                    dyn_str_add(result, c);
                } else {
                    throw_lexical_error("FSM_numerals - F7 - no digit after exponent sign (+/-)");
                }
                break;

            // exponent loop
            case F8:
                if (isdigit(c)) {
                    state = F8;
                    dyn_str_add(result, c);
                } else if (c == '_') {
                    state = F9;
                } else if (c == 'd' || c == 'D') {
                    // dD proceeds to final decision state (F10)
                    state = F10;
                    dyn_str_add(result, c);
                } else {
                    if (isalpha(c)) {
                        // other letters than dD throw error
                        throw_lexical_error("FSM_numerals - F8 - invalid char after exponent");
                    } else {
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
                    dyn_str_add(result, c);
                } else {
                    throw_lexical_error("FSM_numerals - F9 - No digit after '_'");
                }
                break;

            // final decision state
            case F10:
                if (isalpha(c)) {
                    // other letters than dD throw error
                    throw_lexical_error("FSM_numerals - F10 - Invalid char after number");
                } else {
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

/**
 * @brief Handler for identifiers and keywords
 *
 * Handles storing the identifier a check whether it is a keyword
 *
 * @param file The program source file
 * @param sym_table The symbols table of compiler
 * @param first_char The first character of identifier or keyword
 *
 * @return token for either identifier or keyword
 */
Token HANDLER_identifiers(FILE *file, SymTable *sym_table, int first_char) {
    int c;
    DynamicString *id = dyn_str_init();
    dyn_str_add(id, first_char);

    while ((c = fgetc(file)) != EOF) {
        if (string_contains(ENTRY_IDENTIFIERS, c) || isdigit(c)) {
            dyn_str_add(id, c);
        } else {
            ungetc(c, file);
            return is_keyword(id);
        }
    }
    return is_keyword(id);
}

/**
 * @brief Handler for the (at)main annotation
 *
 * @param file The program source file
 * @param sym_table The symbols table of compiler
 *
 * @return token token of type T_AT_MAIN
 */
Token HANDLER_main(FILE *file, SymTable *sym_table) {
    // compare to "main"
    if (fgetc(file) == 'm' && fgetc(file) == 'a' && fgetc(file) == 'i' && fgetc(file) == 'n') {
        // make sure its followed by whitespace
        int c = fgetc(file);
        printf("yes");
        if (string_contains(WHITESPACE_CHARS, c)) {
            printf("yes");

            ungetc(c, file);
            Token token = {T_AT_MAIN, NULL};
            return token;
        }
    }

    throw_lexical_error("HANDLER_main - F6 -  Invalid chars after '@'");
}

/**
 * @brief Handler for operators
 *
 * Handles identifying operators and returning the correct Token
 *
 * @param file The program source file
 * @param sym_table The symbols table of compiler
 * @param first_char The first character of the operator
 *
 * @return token for the operator
 */
Token HANDLER_operators(FILE *file, SymTable *sym_table, int first_char) {
    int second_char = fgetc(file);
    Token token = {NO_TOKEN_FOUND, NULL};

    switch (first_char) {
        case '=':
            if (second_char == '=')
                token.type = T_OP_EQ;
            else if (second_char == '>')
                token.type = T_OP_ROCKET;
            else {
                ungetc(second_char, file);
                token.type = T_OP_ASSIGN;
            }
            return token;
        case '!':
            if (second_char == '=')
                token.type = T_OP_NOT_EQ;
            else {
                ungetc(second_char, file);
                token.type = T_OP_BIN_NOT;
            }
            return token;
        case '>':
            if (second_char == '=')
                token.type = T_OP_GR_EQ;
            else {
                ungetc(second_char, file);
                token.type = T_OP_GR;
            }
            return token;
        case '<':
            if (second_char == '=')
                token.type = T_OP_LESS_EQ;
            else {
                ungetc(second_char, file);
                token.type = T_OP_LESS;
            }
            return token;
        case '+':
            if (second_char == '+')
                token.type = T_OP_PLUS_PLUS;
            else {
                ungetc(second_char, file);
                token.type = T_OP_PLUS;
            }
            return token;
        case '-':
            ungetc(second_char, file);
            token.type = T_OP_MINUS;
            return token;
        case '*':
            ungetc(second_char, file);
            token.type = T_OP_MUL;
            return token;
        case '&':
            if (second_char == '&') {
                token.type = T_OP_BIN_AND;
                return token;
            }
            throw_lexical_error("HANDLER_operators - Invalid char after '&'");
        case '|':
            if (second_char == '|') {
                token.type = T_OP_BIN_OR;
                return token;
            }
            throw_lexical_error("HANDLER_operators - Invalid char after '|'");
        default:
            throw_lexical_error("HANDLER_operators - Invalid char passed into HANDLER_operators - please be smarter next time");
    }
}

/**
 * @brief Handler for other/special tokens
 *
 * @param sym_table The symbols table of compiler
 * @param c The special character
 *
 * @return token for the matched lexem
 */
Token HANDLER_special(SymTable *sym_table, int special_c) {
    Token token = {NO_TOKEN_FOUND, NULL};
    switch (special_c) {
        case ';':
            token.type = T_SEMICOLON;
            break;
        case ',':
            token.type = T_COMMA;
            break;
        case ':':
            token.type = T_COLON;
            break;
        case '(':
            token.type = T_PAR_L;
            break;
        case ')':
            token.type = T_PAR_R;
            break;
        default:
            throw_lexical_error("HANDLER_special - Invalid char passed into HANDLER_special - please be smarter next time");
    }
    return token;
}

/**
 * @brief Handler for whitespace and indentation
 *
 * Handles removing unneeded whitespace and determines the indentation level after a NEWLINE
 *
 * @param file The program source file
 * @param sym_table The symbols table of compiler
 * @param last_token_type The type of the last token passed to caller (parser)
 *
 * @return token token of type T_INDENT with indent level, or of type NO_TOKEN_FOUND for discarded whitespace
 */
Token HANDLER_whitespace(FILE *file, SymTable *sym_table, TokenType last_token_type, int c) {
    Token token = {NO_TOKEN_FOUND, NULL};

    if (c == ' ' && last_token_type == T_NEWLINE) {
        int spaces = 1;
        while ((c = fgetc(file)) == ' ')
            spaces++;

        ungetc(c, file);
        token.type = T_INDENT;
        token_set_indent_count(token, spaces);
    }

    return token;
}

/**
 * @brief provides next token from file
 *
 * Scans file and returns the next token or throws error if no token type matches
 *
 * @warning first run should use T_NEWLINE as last_token_type
 *
 * @param file The program source file
 * @param sym_table The table of symbols of compiler
 *
 * @return token Matched token
 */
Token scanner(FILE *file, SymTable *sym_table, TokenType last_token_type) {
    int c;
    while ((c = fgetc(file)) != EOF) {
        if (string_contains(ENTRY_COMMENTS, c)) {
            Token token;
            if ((token = FSM_comments(file, sym_table)).type == NO_TOKEN_FOUND)
                continue;
            return token;
        }

        if (string_contains(ENTRY_STRINGS, c))
            return FSM_strings(file, sym_table);

        if (string_contains(ENTRY_NUMERALS, c))
            return FSM_numerals(file, sym_table, c);

        if (string_contains(ENTRY_MAIN, c))
            return HANDLER_main(file, sym_table);

        if (string_contains(ENTRY_OPERATORS, c))
            return HANDLER_operators(file, sym_table, c);

        if (string_contains(ENTRY_SPECIAL, c))
            return HANDLER_special(sym_table, c);

        if (string_contains(ENTRY_WHITESPACE, c)) {
            Token token;
            if ((token = HANDLER_whitespace(file, sym_table, last_token_type, c)).type == NO_TOKEN_FOUND)
                continue;
            return token;
        }
        if (string_contains(ENTRY_IDENTIFIERS, c))
            return HANDLER_identifiers(file, sym_table, c);

        throw_lexical_error("scanner() - newest char does not match any token type. Char: '%c', ASCII number: %d");
    }
    Token token = {T_EOF, NULL};
    return token;
}
