
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fake_lexer.h"
#include "parser.h"
#include "ast.h"

static void syntax_error(const char *msg){
    fprintf(stderr, "Syntax error %s\n", msg);
    exit(2);
}

ASTNode* parse_program(){
    parse_prolog();

    ASTNode *prog = create_node(NODE_PROGRAM);
    prog->data.program.functions = NULL;
    prog->data.program.count = 0;
    prog->data.program.capacity = 0;

    while (peek_token().type != TOKEN_EOF){
        ASTNode *prog_tok = parse_function_def();
        ast_array_append(&prog->data.program.functions, &prog->data.program.count, &prog->data.program.capacity, prog_tok);
    }
    return prog;
}


static void parse_prolog(){
    Token t = get_next_token();
    if (t.type != TOKEN_IMPORT){
        syntax_error("expected 'import' prolog");
    } 
    
    Token u = get_next_token();
    if ((u.type != TOKEN_IDENTIFIER)|| strcmp(u.value, "ifj26")!=0){
        syntax_error("expected 'import ifj26.*' in prolog");
    }

    Token dot = get_next_token();
    if(dot.type != TOKEN_DOT){
        syntax_error("expected '.' in prolog");
    }

    Token star = get_next_token();
    if(star.type != TOKEN_STAR){
        syntax_error("expected '*' in prolog");
    }
}

static ASTNode* parse_function_def(){
    Token def_tok = get_next_token();
    if (def_tok.type != TOKEN_DEF){
        syntax_error("No 'def' token");
    }

    Token name_tok = get_next_token();
    if (name_tok.type != TOKEN_IDENTIFIER){
        syntax_error("No 'id' token");
    }

    //TODO parse_params

    char *return_types = NULL;

    Token next = peek_token();
    if (next.type == TOKEN_COLON){
        get_next_token();
        Token check_tok = get_next_token();
            if(check_tok.type != TOKEN_IDENTIFIER){
                syntax_error("Token is not identifier");
            }
        return_types = check_tok.value;
    }

    Token equal_tok = get_next_token();
    if (equal_tok.type != TOKEN_EQUALS){
        syntax_error("No '=' token");
    }
    
    //TODO parse_function_body


    ASTNode *func_def = create_node(NODE_FUNCTION_DEF);
    func_def->data.function_def.name = name_tok.value;
    func_def->data.function_def.parameters = NULL;
    func_def->data.function_def.param_count = 0;
    func_def->data.function_def.body = NULL;
    func_def->data.function_def.return_type = return_types;
    func_def->data.function_def.has_explicit_return_type = (return_types != NULL);
       
    return func_def;
}

static ASTNode* parse_function_header();

static ASTNode* parse_params();

static ASTNode* parse_function_body();

static ASTNode* parse_expr_or_call();

static ASTNode* parse_block();

static ASTNode* parse_statement();

static ASTNode* parse_var_def();

static ASTNode* parse_assignment();

static ASTNode* parse_function_call();

static ASTNode* parse_arguments();

static ASTNode* parse_return();

static ASTNode* parse_if();

static ASTNode* parse_condition();

static ASTNode* parse_while();

//call from preced. parser
ASTNode* parse_expression();
