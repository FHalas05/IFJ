//
// Created by Filip on 04/10/2026.
//

#ifndef IFJ_TOKEN_H
#define IFJ_TOKEN_H


typedef enum {
    NO_TOKEN_FOUND,

    T_IDENT = 1, // start at one to allow NO_TOKEN_FOUND to represent a state that runs the loop again
    T_NEWLINE,
    T_AT_MAIN,
    T_INDENT,

    T_INT,
    T_DOUBLE,
    T_STRING,

    T_PAR_L,
    T_PAR_R,
    T_SEMICOLON,
    T_COLON,
    T_COMMA,
    T_DOT,

    T_KW_DEF,
    T_KW_DO,
    T_KW_DOUBLE,
    T_KW_ELSE,
    T_KW_IF,
    T_KW_IMPORT,
    T_KW_INT,
    T_KW_RETURN,
    T_KW_STRING,
    T_KW_THEN,
    T_KW_UNIT,
    T_KW_VAL,
    T_KW_VAR,
    T_KW_WHILE,
    T_KW_BOOLEAN,
    T_KW_TRUE,
    T_KW_FALSE,
    T_KW_LIST,

    T_OP_PLUS,
    T_OP_MINUS,
    T_OP_MUL,
    T_OP_DIV,
    T_OP_ASSIGN,
    T_OP_EQ,
    T_OP_NOT_EQ,
    T_OP_GR,
    T_OP_GR_EQ,
    T_OP_LESS,
    T_OP_LESS_EQ,
    T_OP_PLUS_PLUS,
    T_OP_ROCKET,
    T_OP_BIN_NOT,
    T_OP_BIN_AND,
    T_OP_BIN_OR,

    T_EOF
} TokenType;


typedef struct {
    TokenType type;;
    void *data;
} Token;


// returns the data stored in INDENT
// Incompatible Token type -> -1
int token_get_indent_count(Token token);
int token_set_indent_count(Token* token, int indent_count);


// returns the data stored in the {IDENT, INT, DOUBLE, STRING} types
// Incompatible Token type -> NULL
char *token_get_data_IDENT_INT_DOUBLE_STRING(Token token);

#endif // IFJ_TOKEN_H
