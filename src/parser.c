#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "scanner.h"
#include "token.h"
#include "parser.h"
#include "ast.h"

#define T_DEDENT ((TokenType)(T_EOF + 1))

static void syntax_error(const char *msg) {
    fprintf(stderr, "Syntax error: %s\n", msg);
    exit(2);
}

/* ===== DOCASNY obal: smazat po merge noveho scanneru ===== */
#define MAX_INDENT 256
static int  indent_stack[MAX_INDENT] = {0};
static int  indent_top = 0;
static int  pending_dedents = 0;
static bool line_start = true;
static bool eof_newline_done = false;
static Token la;
static bool  has_la = false;

static Token make_token(TokenType type) { Token t = {type, NULL}; return t; }

static Token raw_scan(Parser *p) {
    if (has_la) { has_la = false; return la; }
    Token t = scanner(p->src, p->symtab, p->last_scanned);
    p->last_scanned = t.type;
    return t;
}

static Token next_token(Parser *p) {
    if (pending_dedents > 0) { pending_dedents--; return make_token(T_DEDENT); }
    Token t = raw_scan(p);
    if (!line_start) {
        if (t.type == T_NEWLINE) {
            line_start = true;
        } else if (t.type == T_EOF && !eof_newline_done) {
            eof_newline_done = true; la = t; has_la = true; line_start = true;
            return make_token(T_NEWLINE);
        }
        return t;
    }
    int level = 0;
    if (t.type == T_INDENT) {
        level = token_get_indent_count(t);
        t = raw_scan(p);
        if (t.type == T_NEWLINE) return next_token(p);
    }
    if (t.type == T_EOF) { level = 0; eof_newline_done = true; }
    line_start = false; la = t; has_la = true;
    if (level > indent_stack[indent_top]) {
        if (indent_top + 1 >= MAX_INDENT) exit(99);
        indent_stack[++indent_top] = level;
        return make_token(T_INDENT);
    }
    while (level < indent_stack[indent_top]) { indent_top--; pending_dedents++; }
    if (level != indent_stack[indent_top]) syntax_error("inconsistent indentation");
    return next_token(p);
}

static void advance(Parser *p) {
    token_free_data_if_applicable(p->token);   
    p->token = next_token(p);                   
}

static void expect(Parser *p, TokenType type, const char *msg) {
    if (p->token.type != type)
        syntax_error(msg);
    advance(p);
}

static void parse_prolog(Parser *p);

static void parse_function_list(Parser *p);

static void parse_function_def(Parser *p);

static void parse_annotation_opt(Parser *p);

static void parse_annotation_remaining(Parser *p);

static void parse_return_type(Parser *p);

static void parse_type(Parser *p);

static void parse_params(Parser *p);

static void parse_one_param(Parser *p);

static void parse_param_tail(Parser *p);

static void parse_function_body(Parser *p);

static void parse_single_line_body(Parser *p);

static void parse_ident_suffix(Parser *p);

static void parse_args(Parser *p);

static void parse_expr_remaining(Parser *p);

static void parse_binary_op(Parser *p);

static void parse_operand(Parser *p);

static void parse_literal(Parser *p);

static void parse_arg_list(Parser *p);

static void parse_term(Parser *p);

static void parse_args_tail(Parser *p);

static void parse_non_ident_expr_start(Parser *p);

static void parse_block(Parser *p);

static void parse_stmt_list(Parser *p);

static void parse_simple_end(Parser *p);

static void parse_stmt_list_end(Parser *p);

static void parse_simple_stmt(Parser *p);

static void parse_after_ident(Parser *p);

static void parse_return_val(Parser *p);

static void parse_def_value(Parser *p);

static void parse_compound_stmt(Parser *p);

static void parse_else_opt(Parser *p);

static void parse_cond(Parser *p);


void parse_program(FILE *file, SymTable *table) {
    Parser p = { .src = file, .symtab = table, .last_scanned = T_NEWLINE};
    p.token = next_token(&p);          
    switch (p.token.type){
        case T_KW_IMPORT:
            parse_prolog(&p);
            parse_function_list(&p);
            expect(&p, T_EOF, "expected end of file after last function");
            break;
        default:
            syntax_error("Program must start with 'import ifj26.*'");
    }
}

static void parse_prolog(Parser *p){
    switch (p->token.type){
        case T_KW_IMPORT:
            advance(p);
            if ((p->token.type != T_IDENT) || (strcmp((char*) p->token.data, "ifj26") != 0)){
                syntax_error("prolog: expected 'ifj26'");
            }
            advance(p);
            expect(p, T_DOT, "prolog: expected '.'");
            expect(p, T_OP_MUL, "prolog: expected '*'");
            expect(p, T_NEWLINE,"prolog: expected 'import ifj26.*'");
            break;
        default:
            syntax_error("Program must start with 'import'");
    }
}

static void parse_function_list(Parser *p){
    switch (p->token.type){
        case T_AT_MAIN:
        case T_KW_DEF:
            parse_function_def(p);
            parse_function_list(p);
            break;
        case T_EOF:
            break;
        default:
            syntax_error("Expected function definition ('def' or '@main') or end of file");
    }   
}

static void parse_function_def(Parser *p){
    switch (p->token.type){
        case T_AT_MAIN:
        case T_KW_DEF:
            parse_annotation_opt(p);
            expect(p, T_KW_DEF, "function definition: expected 'def'");
            expect(p, T_IDENT, "function definition: expected function name after 'def'");
            expect(p, T_PAR_L, "function definition: expected '(' after function name");
            parse_params(p);
            expect(p, T_PAR_R, "function definition: expected ')' after parameters");
            parse_return_type(p);
            expect(p, T_OP_ASSIGN, "function definition: expected '=' before function body");
            parse_function_body(p);
            break;
        default:
            syntax_error("Function definition: expected 'def' or '@main'");
    }
}

static void parse_annotation_opt(Parser *p){
    switch (p->token.type){
        case T_AT_MAIN:
            advance(p);
            parse_annotation_remaining(p);
            break;
        case T_KW_DEF:
            break;
        default:
            syntax_error("Expected '@main' or 'def'");
    }
}

static void parse_annotation_remaining(Parser *p){
    switch (p->token.type){
        case T_NEWLINE:
            advance(p);
            break;
        case T_KW_DEF:
            break;
    default:
        syntax_error("After '@main': expected newline or 'def'");
    }
}

static void parse_return_type(Parser *p){
    switch (p->token.type){
        case T_COLON:
            advance(p);
            parse_type(p);
            break;
        case T_OP_ASSIGN:
            break;
    default:
        syntax_error("Function header: expected ':' with return type or '='");
    }
}

static void parse_type(Parser *p){
    switch (p->token.type){
        case T_KW_INT:
            advance(p);
            break;
        case T_KW_DOUBLE:
            advance(p);
            break;
        case T_KW_STRING:
            advance(p);
            break;
        case T_KW_UNIT:
            advance(p);
            break;
    default:
        syntax_error("Expected type (Int, Double, String or Unit)");    
    }
}

static void parse_params(Parser *p){
    switch (p->token.type){
        case T_IDENT:
            parse_one_param(p);
            parse_param_tail(p);
            break;
        case T_PAR_R:
            break;
        default:
            syntax_error("Parameter list: expected parameter name or ')'");
    }
}

static void parse_one_param(Parser *p){
    switch (p->token.type){
        case T_IDENT:
            advance(p);
            expect(p, T_COLON, "parameter: expected ':' after parameter name");
            parse_type(p);
            break;
        default:
            syntax_error("Parameter: expected parameter name");
    }
}

static void parse_param_tail(Parser *p){
    switch (p->token.type){
        case T_COMMA:
            advance(p);
            parse_one_param(p);
            parse_param_tail(p);
            break;
        case T_PAR_R:
            break;
        default:
            syntax_error("Parameter list: expected ',' or ')'");
    } 
}

static void parse_function_body(Parser *p){
    switch (p->token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_single_line_body(p);
            expect(p, T_NEWLINE, "function body: expected newline after expression");
            break;
        case T_NEWLINE:
            parse_block(p);
            break;
        default:
            syntax_error("Function body: expected expression or newline with indented block");   
    }
}

static void parse_single_line_body(Parser *p){
    switch (p->token.type){
        case T_IDENT:
            advance(p);
            parse_ident_suffix(p);
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_non_ident_expr_start(p);
            break;
        default:
            syntax_error("Expected expression or function call");
    }
}

static void parse_ident_suffix(Parser *p){
    switch (p->token.type){
        case T_PAR_L:
            advance(p);
            parse_args(p);
            expect(p, T_PAR_R, "function call: expected ')' after arguments");
            break;
        case T_OP_PLUS:
        case T_OP_MINUS:
        case T_OP_MUL:
        case T_OP_DIV:
        case T_OP_EQ:
        case T_OP_NOT_EQ:
        case T_OP_GR:
        case T_OP_GR_EQ:
        case T_OP_LESS:
        case T_OP_LESS_EQ:
        case T_SEMICOLON:
        case T_NEWLINE:
            parse_expr_remaining(p);
            break;
        default:
            syntax_error("After identifier: expected '(', operator or end of statement");
    }
}

static void parse_args(Parser *p){
    switch (p->token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_arg_list(p);
            break;
        case T_PAR_R:
            break;
        default:
            syntax_error("function call: expected argument or ')'");   
    }
}

static void parse_expr_remaining(Parser *p){
    switch (p->token.type){
        case T_OP_PLUS:
        case T_OP_MINUS:
        case T_OP_MUL:
        case T_OP_DIV:
        case T_OP_EQ:
        case T_OP_NOT_EQ:
        case T_OP_GR:
        case T_OP_GR_EQ:
        case T_OP_LESS:
        case T_OP_LESS_EQ:
            parse_binary_op(p);
            parse_operand(p);
            parse_expr_remaining(p);
            break;
        case T_PAR_R:
        case T_SEMICOLON:
        case T_NEWLINE:
        case T_KW_THEN:
        case T_KW_DO:
            break;
        default:
            syntax_error("Expression: expected operator or end of expression");
    }
}

static void parse_binary_op(Parser *p){
    switch (p->token.type){
        case T_OP_PLUS:
        case T_OP_MINUS:
        case T_OP_MUL:
        case T_OP_DIV:
        case T_OP_EQ:
        case T_OP_NOT_EQ:
        case T_OP_GR:
        case T_OP_GR_EQ:
        case T_OP_LESS:
        case T_OP_LESS_EQ:
            advance(p);
            break;
        default:
            syntax_error("Expression: expected binary operator");
    }
}

static void parse_operand(Parser *p){
    switch (p->token.type){
        case T_OP_MINUS:
            advance(p);
            parse_operand(p);
            break;
        case T_IDENT:
            advance(p);
            break;
        case T_PAR_L:
            advance(p);
            if (p->token.type == T_PAR_R){
                advance(p);
            }
            else{
                parse_operand(p);
                parse_expr_remaining(p);
                expect(p, T_PAR_R, "expression: expected ')'");
            }
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_literal(p);
            break;
        default:
            syntax_error("Expression: expected operand (identifier, literal, '-' or '(')");
    }
}

static void parse_literal(Parser *p){
    switch (p->token.type){
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            advance(p);
            break;
        default:
            syntax_error("Expected literal (integer, double or string)");
    }
}

static void parse_arg_list(Parser *p){
    switch (p->token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_term(p);
            parse_args_tail(p);
            break;
        default:
            syntax_error("Function call: expected argument (identifier or literal)");
    }
}

static void parse_term(Parser *p){
    switch (p->token.type){
        case T_IDENT:
            advance(p);
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_literal(p);
            break;
        default:
            syntax_error("Function call: argument must be identifier or literal");
    }
}

static void parse_args_tail(Parser *p){
    switch (p->token.type){
        case T_COMMA:
            advance(p);
            parse_term(p);
            parse_args_tail(p);
            break;
        case T_PAR_R:
            break;
        default:
            syntax_error("Function call: expected ',' or ')'");
    }
}

static void parse_non_ident_expr_start(Parser *p){
    switch (p->token.type){
        case T_OP_MINUS:
            advance(p);
            parse_operand(p);
            parse_expr_remaining(p);
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_literal(p);
            parse_expr_remaining(p);
            break;
        case T_PAR_L:
            advance(p);
            if (p->token.type == T_PAR_R){
                advance(p);
            }
            else{
                parse_operand(p);
                parse_expr_remaining(p);
                expect(p, T_PAR_R, "expression: expected ')'");
            }
            parse_expr_remaining(p);
            break;
        default:
            syntax_error("Expression: expected literal, '-' or '('");
    }
}

static void parse_block(Parser *p){
    switch (p->token.type){
        case T_NEWLINE:
            advance(p);
            expect(p, T_INDENT, "block: expected indented block");
            parse_stmt_list(p);
            break;
        default:
            syntax_error("Block: expected newline and indented block");
    }
}

static void parse_stmt_list(Parser *p){
    switch (p->token.type){
        case T_KW_VAL:
        case T_KW_VAR:
        case T_KW_RETURN:
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_simple_stmt(p);
            parse_simple_end(p);
            break;
        case T_KW_IF:
        case T_KW_WHILE:
            parse_compound_stmt(p);
            parse_stmt_list_end(p);
            break;
        default:
            syntax_error("Block: expected statement");
    }
}

static void parse_simple_end(Parser *p){
    switch (p->token.type){
        case T_SEMICOLON:
            advance(p);
            parse_stmt_list(p);
            break;
        case T_NEWLINE:
            advance(p);
            parse_stmt_list_end(p);
            break;
        default:
            syntax_error("Statement: expected ';' or newline after statement");
    }
}

static void parse_stmt_list_end(Parser *p){
    switch (p->token.type){
        case T_KW_VAL:
        case T_KW_VAR:
        case T_KW_RETURN:
        case T_KW_IF:
        case T_KW_WHILE:
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_stmt_list(p);
            break;
        case T_DEDENT:
            advance(p);
            break;
        default:
            syntax_error("Block: expected next statement or end of block");
    }
}

static void parse_simple_stmt(Parser *p){
    switch (p->token.type){
        case T_KW_VAL:
        case T_KW_VAR:
            parse_def_value(p);
            expect(p, T_IDENT, "variable definition: expected variable name");
            parse_return_type(p);
            expect(p, T_OP_ASSIGN, "variable definition: expected '='");
            parse_single_line_body(p);
            break;
        case T_IDENT:
            advance(p);
            parse_after_ident(p);
            break;
        case T_KW_RETURN:
            advance(p);
            parse_return_val(p);
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_non_ident_expr_start(p);
            break;
        default:
            syntax_error("Expected statement (val, var, return, assignment, call or expression)");   
    }
}

static void parse_after_ident(Parser *p){
    switch (p->token.type){
        case T_OP_ASSIGN:
            advance(p);
            parse_single_line_body(p);
            break;
        case T_OP_PLUS:
        case T_OP_MINUS:
        case T_OP_MUL:
        case T_OP_DIV:
        case T_OP_EQ:
        case T_OP_NOT_EQ:
        case T_OP_GR:
        case T_OP_GR_EQ:
        case T_OP_LESS:
        case T_OP_LESS_EQ:
        case T_PAR_L:
        case T_SEMICOLON:
        case T_NEWLINE:
            parse_ident_suffix(p);
            break;
        default:
            syntax_error("After identifier: expected '=', '(', operator or end of statement");
    }
}

static void parse_return_val(Parser *p){
    switch (p->token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_single_line_body(p);
            break;
        case T_SEMICOLON:
        case T_NEWLINE:
            break;
        default:
            syntax_error("Return: expected expression or end of statement");
    }
}

static void parse_def_value(Parser *p){
    switch (p->token.type){
        case T_KW_VAL:
        case T_KW_VAR:
            advance(p);
            break;
        default:
            syntax_error("Variable definition: expected 'val' or 'var'");
    }
}

static void parse_compound_stmt(Parser *p){
    switch (p->token.type){
        case T_KW_IF:
            advance(p);
            parse_cond(p);
            expect(p, T_KW_THEN, "if: expected 'then' after condition");
            parse_block(p);
            expect(p, T_KW_ELSE, "if: expected 'else'");
            parse_block(p);
            break;
        case T_KW_WHILE:
            advance(p);
            parse_cond(p);
            expect(p, T_KW_DO, "while: expected 'do' after condition");
            parse_block(p);
            break;
        default:
            syntax_error("Expected 'if' or 'while'");
    }
}

/*
static void parse_else_opt(Parser *p){
    switch (p->token.type){
        case T_KW_ELSE:
            advance(p);
            parse_block(p);
            break;
        case T_KW_VAL:
        case T_KW_VAR:
        case T_KW_RETURN:
        case T_KW_IF:
        case T_KW_WHILE:
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
        case T_DEDENT:
            break;
        default:
            syntax_error("After if-block: expected 'else', next statement or end of block");
    }
}*/

static void parse_cond(Parser *p){
    switch (p->token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_operand(p);
            parse_expr_remaining(p);
            break;
        default:
            syntax_error("Condition: expected expression");
    }
}