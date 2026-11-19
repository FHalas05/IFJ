//
// Created by Filip on 04/10/2026.
//

#include "scanner.h"
#include "dynamic_string.h"
#include "keywords.h"
#include "library.h"
#include "token.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ENTRY_COMMENTS "/"
#define ENTRY_STRINGS "\""
#define ENTRY_NUMERALS ".0123456789"
#define ENTRY_MAIN "@"
#define ENTRY_OPERATORS "=!><+-*&|"
#define ENTRY_SPECIAL ";,:()\n"
#define ENTRY_WHITESPACE " \t\v\f\r"
#define ENTRY_IDENTIFIERS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"

#define throw_lexical_error(description) throw_fatal_error(1, description)

/**
 * @brief FSM for comments and division
 *
 * This FSM handles tokens for division ('/') , one line comments and block comments
 *
 * @param file The program source file
 * @param last_token_type The type of the last token passed to caller (parser)
 *
 * @return Token for '/' , one line comments and block comments
 */
Token FSM_comments(FILE *file, TokenType last_token_type) {
    typedef enum { F1, F2, F3, F4, F5, F6 } State;

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
                Token token = {NO_TOKEN_FOUND, NULL};
                if (last_token_type != T_NEWLINE) {
                    token.type = T_NEWLINE;
                }
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
        // additional nesting sequence
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
        // additional denesting sequence
        case F5:
            if (c == '/') {
                state = F3;
                nesting_depth--;
                if (nesting_depth == 0 && new_line_check) {
                    state = F6;
                } else if (nesting_depth == 0 && !new_line_check) {
                    Token token = {NO_TOKEN_FOUND, NULL};
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
        // Checks to ensure that there is nothing but whitespace after a block comment followed by a
        // newline
        case F6:
            if (c == ' ' || c == '\t' || c == '\r' || c == '\v') {
                state = F6;
            } else if (c == '\n') {
                Token token = {T_NEWLINE, NULL};
                return token;
            } else {
                throw_lexical_error("FSM_comments - F6 - Unexpected character after block comment "
                                    "on the same line");
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
 *
 * @return Token for one-line string or multiple-line strings
 */
Token FSM_strings(FILE *file) {
    typedef enum { F1, F2, F3, F4, F5, F6 } State;

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
                throw_lexical_error("FSM_strings - F1 - Unfinished string literal");
            } else if (c == '"' && !line_string) {
                // possible multiple-line string or empty string
                state = F2;
            } else if (c == '"' && line_string) {
                // final one line string
                Token token = {T_STRING, dyn_str_get_string_copy(final_string)};
                dyn_str_free(final_string);
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
                Token token = {T_STRING, dyn_str_get_string_copy(final_string)};
                dyn_str_free(final_string);
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
                Token token = {T_STRING, dyn_str_get_string_copy(final_string)};
                dyn_str_free(final_string);
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
                throw_lexical_error("FSM_strings - F6 - Invalid escape sequence");
            }
            break;
        }
    }
    if (state == F2) {
        Token token = {T_STRING, dyn_str_get_string_copy(final_string)};
        dyn_str_free(final_string);
        return token;
    }
    dyn_str_free(final_string);
    throw_lexical_error("FSM_strings - end state");
    // throw_lexical_error exits the program, but the compiler still expects a return here
    return (Token){NO_TOKEN_FOUND, NULL};
}

/**
 * @brief FSM for numerals (int, double)
 *
 * This FSM handles all numeral types and the dot operator
 *
 * @param file The program source file
 * @param first_char The first char of the supposed numeral
 *
 * @return Token of numeral with its content
 */
Token FSM_numerals(FILE *file, int first_char) {
    typedef enum { F1, F2, F3, F4, F5, F6, F7, F8, F9, F10 } State;

    State state = first_char == '.'
                      ? F3
                      : F1; // if first char is ".", go straight to double entry point (F3), else F1
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
                if ((first_char == '0' &&
                     result->len != 1) || // cannot start with 0 and be something else than 0
                    isalpha(c)) {
                    // cannot be a letter (except {E,e} - handled above)
                    dyn_str_free(result);
                    throw_lexical_error("FSM_numerals - F1 - Nonzero number starts with 0, OR an "
                                        "invalid letter follows the number");
                } else {
                    // return result
                    ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                    Token token = {T_INT, dyn_str_get_string_copy(result)};
                    dyn_str_free(result);
                    return token;
                }
            }
            break;

        // _ in INT
        case F2:
            if (isdigit(c)) {
                state = F1;
                dyn_str_add(result, c);
            } else if (c == '_') {
                state = F2;
            } else {
                // there must always be a digit after '_'
                dyn_str_free(result);
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
                // if the first char was '.' and there's no number before and after it, then it must
                // be the dot operator
                if (first_char == '.' && result->len == 1) {
                    ungetc(c, file);
                    Token token = {T_DOT, result};
                    dyn_str_free(result);
                    return token;
                }
                throw_lexical_error(
                    "FSM_numerals - F3 - there is no digit after a decimal point"); // if it's
                                                                                    // something
                                                                                    // else than '.'
                                                                                    // or a numeral,
                                                                                    // its invalid
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
                    dyn_str_free(result);
                    throw_lexical_error("FSM_numerals - F4 - Invalid char after number");
                } else {
                    ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                    Token token = {T_DOUBLE, dyn_str_get_string_copy(result)};
                    dyn_str_free(result);
                    return token;
                }
            }
            break;

        // _ for mantissa
        case F5:
            if (isdigit(c)) {
                state = F4;
                dyn_str_add(result, c);
            } else if (c == '_') {
                state = F5;
            } else {
                dyn_str_free(result);
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
                dyn_str_free(result);
                throw_lexical_error("FSM_numerals - F6 - Invalid char after exponent (e/E)");
            }
            break;

        // exponent sign
        case F7:
            if (isdigit(c)) {
                state = F8;
                dyn_str_add(result, c);
            } else {
                dyn_str_free(result);
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
                    dyn_str_free(result);
                    throw_lexical_error("FSM_numerals - F8 - invalid char after exponent");
                } else {
                    ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                    Token token = {T_DOUBLE, dyn_str_get_string_copy(result)};
                    dyn_str_free(result);
                    return token;
                }
            }
            break;

        // _ for exponent
        case F9:
            if (isdigit(c)) {
                state = F8;
                dyn_str_add(result, c);
            } else if (c == '_') {
                state = F9;
            } else {
                dyn_str_free(result);
                throw_lexical_error("FSM_numerals - F9 - No digit after '_'");
            }
            break;

        // final decision state
        case F10:
            if (isalnum(c)) {
                // other letters than dD throw error
                dyn_str_free(result);
                throw_lexical_error("FSM_numerals - F10 - Invalid char after number");
            } else {
                ungetc(c, file); // cannot be left unhandled (could be an operator etc.)
                Token token = {T_DOUBLE, dyn_str_get_string_copy(result)};
                dyn_str_free(result);
                return token;
            }
        }
    }
}

/**
 * @brief Handler for identifiers and keywords
 *
 * Handles storing the identifier a check whether it is a keyword
 *
 * @param file The program source file
 * @param first_char The first character of identifier or keyword
 *
 * @return Token for either identifier or keyword
 */
Token HANDLER_identifiers(FILE *file, int first_char) {
    int c;
    DynamicString *id = dyn_str_init();
    dyn_str_add(id, first_char);

    while ((c = fgetc(file)) != EOF) {
        if (string_contains(ENTRY_IDENTIFIERS, c) || isdigit(c)) {
            dyn_str_add(id, c);
        } else {
            ungetc(c, file);
            return get_token_for_KEYWORD_or_IDENT(id);
        }
    }
    return get_token_for_KEYWORD_or_IDENT(id);
}

/**
 * @brief Handler for the (at)main annotation
 *
 * @param file The program source file
 *
 * @return Token of type T_AT_MAIN
 */
Token HANDLER_main(FILE *file) {
    // compare to "main"
    if (fgetc(file) == 'm' && fgetc(file) == 'a' && fgetc(file) == 'i' && fgetc(file) == 'n') {
        // make sure its followed by whitespace
        int c = fgetc(file);
        if (c == EOF || c == '/' || string_contains(" \t\n\v\f\r", c)) {
            if (c != EOF) {
                ungetc(c, file);
            }
            Token token = {T_AT_MAIN, NULL};
            return token;
        }
        throw_lexical_error("HANDLER_main - Invalid chars after '@main', could be EOF as well");
    }

    throw_lexical_error("HANDLER_main - Invalid chars after '@'");
    // throw_lexical_error exits the program, but the compiler still expects a return here
    return (Token){NO_TOKEN_FOUND, NULL};
}

/**
 * @brief Handler for operators
 *
 * Handles identifying operators and returning the correct Token
 *
 * @param file The program source file
 * @param first_char The first character of the operator
 *
 * @return Token for the operator
 */
Token HANDLER_operators(FILE *file, int first_char) {
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
            token.type = T_OP_NOT;
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
        break;
    case '|':
        if (second_char == '|') {
            token.type = T_OP_BIN_OR;
            return token;
        }
        throw_lexical_error("HANDLER_operators - Invalid char after '|'");
        break;
    default:
        throw_lexical_error("HANDLER_operators - Invalid char passed into HANDLER_operators - "
                            "please be smarter next time");
    }

    return token;
}

/**
 * @brief Handler for other/special tokens
 *
 * @param last_token_type The type of the last token passed to caller (parser)
 * @param special_c The special character
 *
 * @return Token for the matched lexeme
 */
Token HANDLER_special(TokenType last_token_type, int special_c) {
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
    case '\n':
        if (last_token_type != T_NEWLINE) { // two newlines after one another are just pointless
            token.type = T_NEWLINE;
        }
        break;
    default:
        fprintf(stderr, "The char ASCII number for the following error is: %d.\n", special_c);
        throw_lexical_error("HANDLER_special - Invalid char passed into HANDLER_special - please "
                            "be smarter next time");
    }
    return token;
}

/**
 * @brief Handler for whitespace and indentation
 *
 * Handles removing unneeded whitespace and determines the indentation level after a NEWLINE
 *
 * @param file The program source file
 * @param last_token_type The type of the last token passed to caller (parser)
 * @param c The supposed whitespace char
 *
 * @return Token of type T_INDENT with indent level, or of type NO_TOKEN_FOUND for discarded
 * whitespace
 */
Token HANDLER_whitespace(FILE *file, TokenType last_token_type, int c) {
    Token token = {NO_TOKEN_FOUND, NULL};

    if (c == '\r') {
        return token; // just always discard
    }

    if (c == '\t' && last_token_type == T_NEWLINE) { // if the line starts with a tab
        while (string_contains(" \t\r\v\f\r", c)) {
            c = fgetc(file);
        } // skip all whitespace if the line starts with a tab - to find out whether it is empty
        if (c == EOF) {
            token.type = T_EOF;
            return token;
        }
        if (c != '\n') {
            throw_lexical_error("HANDLER_whitespace - tab at the start if an unempty line");
        } else {
            return token;
        } // discarding line if it starts with tab and is empty
    }

    if (c == ' ' && last_token_type == T_NEWLINE) {
        int spaces = 1;
        bool has_tab = false;
        while ((c = fgetc(file)) == ' ')
            spaces++;

        // skip remaining whitespaces and remember if it contains a tab
        while (string_contains(" \t\v\f\r", c)) {
            if (c == '\t')
                has_tab = true;

            c = fgetc(file);
        }

        // if theres newline right after indent and or other whitespace, the whole line can be
        // discarded
        if (c == '\n')
            return token;

        if (c == EOF) {
            token.type = T_EOF;
            return token;
        }

        if (has_tab) {
            throw_lexical_error("Tab in indentation of a nonempty line");
        }

        ungetc(c, file);
        token.type = T_INDENT;
        token_set_indent_count(&token, spaces);
    } else if (last_token_type == T_NEWLINE) { // no non-space whitespace is allowed after a newline
        throw_lexical_error(
            "HANDLER_whitespace - Disallowed whitespace after newline - (probably a tab)");
    }

    return token;
}

/**
 * @brief Returns next token from file
 *
 * Scans file and returns the next token or throws error if no token type matches
 *
 * @warning first run should use T_NEWLINE as last_token_type
 * @warning you MUST free the data in tokens which include additional data in string form
 * i.e. free(token.data)
 *
 * @param file The program source file
 *
 * @return Matched token
 */
Token scanner(FILE *file) {
    int c;

    static TokenType last_token_type = T_NEWLINE;

    Token token = {T_EOF, NULL};

    while ((c = fgetc(file)) != EOF) {
        if (string_contains(ENTRY_COMMENTS, c))
            token = FSM_comments(file, last_token_type);

        else if (string_contains(ENTRY_STRINGS, c))
            token = FSM_strings(file);

        else if (string_contains(ENTRY_NUMERALS, c))
            token = FSM_numerals(file, c);

        else if (string_contains(ENTRY_MAIN, c))
            token = HANDLER_main(file);

        else if (string_contains(ENTRY_OPERATORS, c))
            token = HANDLER_operators(file, c);

        else if (string_contains(ENTRY_SPECIAL, c))
            token = HANDLER_special(last_token_type, c);

        else if (string_contains(ENTRY_WHITESPACE, c))
            token = HANDLER_whitespace(file, last_token_type, c);

        else if (string_contains(ENTRY_IDENTIFIERS, c))
            token = HANDLER_identifiers(file, c);

        else {
            fprintf(stderr, "The ASCII number of the char causing the following error is: %d", c);
            throw_lexical_error("scanner() - newest char does not match any token type");
        }

        if (token.type == NO_TOKEN_FOUND)
            continue;

        last_token_type = token.type;
        return token;
    }

    token.type = T_EOF;
    return token;
}
