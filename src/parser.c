#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "scanner.h"
#include "token.h"
#include "parser.h"
#include "ast.h"


#define T_DEDENT ((TokenType)(T_EOF + 1))
#define MAX_INDENT 256

static void syntax_error(const char *msg) {
    fprintf(stderr, "Syntax error: %s\n", msg);
    exit(2);
}

/* ===== Obal kolem scanneru: z T_INDENT(n) dela INDENT / DEDENT ===== */

static FILE *src;                         
static SymTable *symtab;                   
static TokenType last_scanned = T_NEWLINE; 

static int  indent_stack[MAX_INDENT] = {0}; 
static int  indent_top = 0;
static int  pending_dedents = 0;            
static bool line_start = true;              
static bool eof_newline_done = false;

static Token la;                            
static bool  has_la = false;

static Token make_token(TokenType type) {
    Token t = {type, NULL};
    return t;
}

static Token raw_scan(void) {
    if (has_la) {
        has_la = false;
        return la;
    }
    Token t = scanner(src, symtab, last_scanned);
    last_scanned = t.type;
    return t;
}

static Token next_token(void) {
    if (pending_dedents > 0) {
        pending_dedents--;
        return make_token(T_DEDENT);
    }

    Token t = raw_scan();

    if (!line_start) {
        if (t.type == T_NEWLINE) {
            line_start = true;
        } else if (t.type == T_EOF && !eof_newline_done) {
            eof_newline_done = true;
            la = t;
            has_la = true;
            line_start = true;
            return make_token(T_NEWLINE);
        }
        return t;
    }

    
    int level = 0;
    if (t.type == T_INDENT) {
        level = token_get_indent_count(t);
        t = raw_scan();
        if (t.type == T_NEWLINE)        
            return next_token();
    }
    if (t.type == T_EOF) {
        level = 0;                      
        eof_newline_done = true;
    }

    line_start = false;
    la = t;                             
    has_la = true;

    if (level > indent_stack[indent_top]) {          
        if (indent_top + 1 >= MAX_INDENT)
            exit(99);
        indent_stack[++indent_top] = level;
        return make_token(T_INDENT);
    }
    while (level < indent_stack[indent_top]) {       
        indent_top--;
        pending_dedents++;
    }
    if (level != indent_stack[indent_top])           
        syntax_error("inconsistent indentation");

    return next_token();   
}

static Token token;          

static void advance(void) {
    token_free_data_if_applicable(token);   
    token = next_token();                   
}

static void expect(TokenType type, const char *msg) {
    if (token.type != type)
        syntax_error(msg);
    advance();
}

static void parse_prolog(void);

static void parse_function_list(void);

static void parse_function_def(void);

static void parse_annotation_opt(void);

static void parse_annotation_remaining(void);

static void parse_return_type(void);

static void parse_type(void);

static void parse_params(void);

static void parse_one_param(void);

static void parse_param_tail(void);

static void parse_function_body(void);

static void parse_single_line_body(void);

static void parse_ident_suffix(void);

static void parse_args(void);

static void parse_expr_remaining(void);

static void parse_binary_op(void);

static void parse_operand(void);

static void parse_literal(void);

static void parse_arg_list(void);

static void parse_term(void);

static void parse_args_tail(void);

static void parse_non_ident_expr_start(void);

static void parse_block(void);

static void parse_stmt_list(void);

static void parse_simple_end(void);

static void parse_stmt_list_end(void);

static void parse_simple_stmt(void);

static void parse_after_ident(void);

static void parse_return_val(void);

static void parse_def_value(void);

static void parse_compound_stmt(void);

static void parse_else_opt(void);

static void parse_cond(void);


void parse_program(FILE *file, SymTable *table) {
    src = file;              
    symtab = table;
    advance();               
    switch (token.type){
        case T_KW_IMPORT:
            parse_prolog();
            parse_function_list();
            expect(T_EOF, "expected end of file after last function");
            break;
        default:
            syntax_error("Program must start with 'import ifj26.*'");
    }
}

static void parse_prolog(void){
    switch (token.type){
        case T_KW_IMPORT:
            advance();
            expect(T_IDENT, "prolog: expected 'ifj26'");
            expect(T_DOT, "prolog: expected '.'");
            expect(T_OP_MUL, "prolog: expected '*'");
            expect(T_NEWLINE,"prolog: expected 'import ifj26.*'");
            break;
        default:
            syntax_error("Program must start with 'import'");
    }
}

static void parse_function_list(void){
    switch (token.type){
        case T_AT_MAIN:
        case T_KW_DEF:
            parse_function_def();
            parse_function_list();
            break;
        case T_EOF:
            break;
        default:
            syntax_error("Expected function definition ('def' or '@main') or end of file");
    }   
}

static void parse_function_def(void){
    switch (token.type){
        case T_AT_MAIN:
        case T_KW_DEF:
            parse_annotation_opt();
            expect(T_KW_DEF, "function definition: expected 'def'");
            expect(T_IDENT, "function definition: expected function name after 'def'");
            expect(T_PAR_L, "function definition: expected '(' after function name");
            parse_params();
            expect(T_PAR_R, "function definition: expected ')' after parameters");
            parse_return_type();
            expect(T_OP_ASSIGN, "function definition: expected '=' before function body");
            parse_function_body();
            break;
        default:
            syntax_error("Function definition: expected 'def' or '@main'");
    }
}

static void parse_annotation_opt(void){
    switch (token.type){
        case T_AT_MAIN:
            advance();
            parse_annotation_remaining();
            break;
        case T_KW_DEF:
            break;
        default:
            syntax_error("Expected '@main' or 'def'");
    }
}

static void parse_annotation_remaining(void){
    switch (token.type){
        case T_NEWLINE:
            advance();
            break;
        case T_KW_DEF:
            break;
    default:
        syntax_error("After '@main': expected newline or 'def'");
    }
}

static void parse_return_type(void){
    switch (token.type){
        case T_COLON:
            advance();
            parse_type();
            break;
        case T_OP_ASSIGN:
            break;
    default:
        syntax_error("Function header: expected ':' with return type or '='");
    }
}

static void parse_type(void){
    switch (token.type){
        case T_KW_INT:
            advance();
            break;
        case T_KW_DOUBLE:
            advance();
            break;
        case T_KW_STRING:
            advance();
            break;
        case T_KW_UNIT:
            advance();
            break;
    default:
        syntax_error("Expected type (Int, Double, String or Unit)");    
    }
}

static void parse_params(void){
    switch (token.type){
        case T_IDENT:
            parse_one_param();
            parse_param_tail();
            break;
        case T_PAR_R:
            break;
        default:
            syntax_error("Parameter list: expected parameter name or ')'");
    }
}

static void parse_one_param(void){
    switch (token.type){
        case T_IDENT:
            advance();
            expect(T_COLON, "parameter: expected ':' after parameter name");
            parse_type();
            break;
        default:
            syntax_error("Parameter: expected parameter name");
    }
}

static void parse_param_tail(void){
    switch (token.type){
        case T_COMMA:
            advance();
            parse_one_param();
            parse_param_tail();
            break;
        case T_PAR_R:
            break;
        default:
            syntax_error("Parameter list: expected ',' or ')'");
    } 
}

static void parse_function_body(void){
    switch (token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_single_line_body();
            expect(T_NEWLINE, "function body: expected newline after expression");
            break;
        case T_NEWLINE:
            parse_block();
            break;
        default:
            syntax_error("Function body: expected expression or newline with indented block");   
    }
}

static void parse_single_line_body(void){
    switch (token.type){
        case T_IDENT:
            advance();
            parse_ident_suffix();
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_non_ident_expr_start();
            break;
        default:
            syntax_error("Expected expression or function call");
    }
}

static void parse_ident_suffix(void){
    switch (token.type){
        case T_PAR_L:
            advance();
            parse_args();
            expect(T_PAR_R, "function call: expected ')' after arguments");
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
            parse_expr_remaining();
            break;
        default:
            syntax_error("After identifier: expected '(', operator or end of statement");
    }
}

static void parse_args(void){
    switch (token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_arg_list();
            break;
        case T_PAR_R:
            break;
        default:
            syntax_error("function call: expected argument or ')'");   
    }
}

static void parse_expr_remaining(void){
    switch (token.type){
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
            parse_binary_op();
            parse_operand();
            parse_expr_remaining();
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

static void parse_binary_op(void){
    switch (token.type){
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
            advance();
            break;
        default:
            syntax_error("Expression: expected binary operator");
    }
}

static void parse_operand(void){
    switch (token.type){
        case T_OP_MINUS:
            advance();
            parse_operand();
            break;
        case T_IDENT:
            advance();
            break;
        case T_PAR_L:
            advance();
            parse_operand();
            parse_expr_remaining();
            expect(T_PAR_R, "expression: expected ')'");
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_literal();
            break;
        default:
            syntax_error("Expression: expected operand (identifier, literal, '-' or '(')");
    }
}

static void parse_literal(void){
    switch (token.type){
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            advance();
            break;
        default:
            syntax_error("Expected literal (integer, double or string)");
    }
}

static void parse_arg_list(void){
    switch (token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_term();
            parse_args_tail();
            break;
        default:
            syntax_error("Function call: expected argument (identifier or literal)");
    }
}

static void parse_term(void){
    switch (token.type){
        case T_IDENT:
            advance();
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_literal();
            break;
        default:
            syntax_error("Function call: argument must be identifier or literal");
    }
}

static void parse_args_tail(void){
    switch (token.type){
        case T_COMMA:
            advance();
            parse_term();
            parse_args_tail();
            break;
        case T_PAR_R:
            break;
        default:
            syntax_error("Function call: expected ',' or ')'");
    }
}

static void parse_non_ident_expr_start(void){
    switch (token.type){
        case T_OP_MINUS:
            advance();
            parse_operand();
            parse_expr_remaining();
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
            parse_literal();
            parse_expr_remaining();
            break;
        case T_PAR_L:
            advance();
            parse_operand();
            parse_expr_remaining();
            expect(T_PAR_R, "expression: expected ')'");
            parse_expr_remaining();
            break;
        default:
            syntax_error("Expression: expected literal, '-' or '('");
    }
}

static void parse_block(void){
    switch (token.type){
        case T_NEWLINE:
            advance();
            expect(T_INDENT, "block: expected indented block");
            parse_stmt_list();
            break;
        default:
            syntax_error("Block: expected newline and indented block");
    }
}

static void parse_stmt_list(void){
    switch (token.type){
        case T_KW_VAL:
        case T_KW_VAR:
        case T_KW_RETURN:
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_simple_stmt();
            parse_simple_end();
            break;
        case T_KW_IF:
        case T_KW_WHILE:
            parse_compound_stmt();
            parse_stmt_list_end();
            break;
        default:
            syntax_error("Block: expected statement");
    }
}

static void parse_simple_end(void){
    switch (token.type){
        case T_SEMICOLON:
            advance();
            parse_stmt_list();
            break;
        case T_NEWLINE:
            advance();
            parse_stmt_list_end();
            break;
        default:
            syntax_error("Statement: expected ';' or newline after statement");
    }
}

static void parse_stmt_list_end(void){
    switch (token.type){
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
            parse_stmt_list();
            break;
        case T_DEDENT:
            advance();
            break;
        default:
            syntax_error("Block: expected next statement or end of block");
    }
}

static void parse_simple_stmt(void){
    switch (token.type){
        case T_KW_VAL:
        case T_KW_VAR:
            parse_def_value();
            expect(T_IDENT, "variable definition: expected variable name");
            parse_return_type();
            expect(T_OP_ASSIGN, "variable definition: expected '='");
            parse_single_line_body();
            break;
        case T_IDENT:
            advance();
            parse_after_ident();
            break;
        case T_KW_RETURN:
            advance();
            parse_return_val();
            break;
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_non_ident_expr_start();
            break;
        default:
            syntax_error("Expected statement (val, var, return, assignment, call or expression)");   
    }
}

static void parse_after_ident(void){
    switch (token.type){
        case T_OP_ASSIGN:
            advance();
            parse_single_line_body();
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
            parse_ident_suffix();
            break;
        default:
            syntax_error("After identifier: expected '=', '(', operator or end of statement");
    }
}

static void parse_return_val(void){
    switch (token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_single_line_body();
            break;
        case T_SEMICOLON:
        case T_NEWLINE:
            break;
        default:
            syntax_error("Return: expected expression or end of statement");
    }
}

static void parse_def_value(void){
    switch (token.type){
        case T_KW_VAL:
        case T_KW_VAR:
            advance();
            break;
        default:
            syntax_error("Variable definition: expected 'val' or 'var'");
    }
}

static void parse_compound_stmt(void){
    switch (token.type){
        case T_KW_IF:
            advance();
            parse_cond();
            expect(T_KW_THEN, "if: expected 'then' after condition");
            parse_block();
            parse_else_opt();
            break;
        case T_KW_WHILE:
            advance();
            parse_cond();
            expect(T_KW_DO, "while: expected 'do' after condition");
            parse_block();
            break;
        default:
            syntax_error("Expected 'if' or 'while'");
    }
}

static void parse_else_opt(void){
    switch (token.type){
        case T_KW_ELSE:
            advance();
            parse_block();
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
}

static void parse_cond(void){
    switch (token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:
            parse_operand();
            parse_expr_remaining();
            break;
        default:
            syntax_error("Condition: expected expression");
    }
}