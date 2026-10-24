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

static void expect(TokenType type) {
    if (token.type != type)
        syntax_error("unexpected token");
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
            expect(T_EOF);
            break;
        default:
            syntax_error("Program must start with import");
    }
}

static void parse_prolog(void){
    switch (token.type){
        case T_KW_IMPORT:
            advance();
            expect(T_IDENT);
            expect(T_DOT);
            expect(T_OP_MUL);
            expect(T_NEWLINE);
            break;
        default:
            syntax_error("Program must start with import");
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
            syntax_error("Expected @main, def or eof");
    }   
}

static void parse_function_def(void){
    switch (token.type){
        case T_AT_MAIN:
        case T_KW_DEF:
            parse_annotation_opt();
            expect(T_KW_DEF);
            expect(T_IDENT);
            expect(T_PAR_L);
            parse_params();
            expect(T_PAR_R);
            parse_return_type();
            expect(T_OP_ASSIGN);
            parse_function_body();
            break;
        default:
            syntax_error("No function definition");
    }
}

static void parse_annotation_opt(void){
    switch (token.type){
        case T_AT_MAIN:
            expect(T_AT_MAIN);
            parse_annotation_remaining();
            break;
        case T_KW_DEF:
            break;
        default:
            syntax_error("No annotation option");
    }
}
