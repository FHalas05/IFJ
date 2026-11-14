#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "scanner.h"
#include "token.h"
#include "library.h"
#include "symtable.h"
#include "parser.h"
#include "ast.h"
#include "expression_parser.h"

static void syntax_error(const char *msg) {
    fprintf(stderr, "Syntax error: %s\n", msg);
    exit(2);
}

static void init_parser(Parser *p, FILE *file, SymTable *table) {
    p->src = file;
    p->symtab = table;
    p->has_peek = false;
    p->has_pending_token = false;
    p->indent_capacity = 16;
    p->indent_count = 0;
    p->pending_dedent = 0;
    p->line_start = true;
    p->eof_newline_done = false;
    p->paren_depth = 0;
    p->logical_indent = 0;
    p->pending_lf = false;

    p->indent_stack = (int *)malloc(p->indent_capacity * sizeof(int));
    if(p->indent_stack == NULL) {
        throw_fatal_error(99, "Could not allocate memory!");
    }

    p->indent_stack[p->indent_count++] = 0;
}

static void push_indent(Parser *p, int level) {
    if(p->indent_count == p->indent_capacity) {
        size_t new_capacity = p->indent_capacity * 2;
        int *new_stack = realloc(p->indent_stack, new_capacity * sizeof(int));
        if(new_stack == NULL) {
            throw_fatal_error(99, "Could not allocate memory!");
        }
        p->indent_stack = new_stack;
        p->indent_capacity = new_capacity;
    }

    p->indent_stack[p->indent_count++] = level;
}

static Token make_token(TokenType type) { 
    Token t = {type, NULL}; 
    return t; 
}

static Token raw_scan(Parser *p) {
    if (p->has_pending_token) { 
        p->has_pending_token = false;
        return p->pending_token; 
    }
    Token t = scanner(p->src);
    return t;
}

static Token next_token(Parser *p) {
    if (p->pending_dedent > 0) { 
        p->pending_dedent--; 
        return make_token(T_DEDENT); 
    }
    Token t = raw_scan(p);

    if (!p->line_start) {
        if (t.type == T_NEWLINE) {
            p->line_start = true;

            if(p->paren_depth > 0) {
                p->pending_lf = true;
                return next_token(p);
            }
        } else if (t.type == T_EOF && !p->eof_newline_done) {
            p->eof_newline_done = true; 
            p->pending_token = t; 
            p->has_pending_token = true; 
            p->line_start = true;
            return make_token(T_NEWLINE);
        }

        if (t.type == T_PAR_L) {
            p->paren_depth++;
        } else if (t.type == T_PAR_R) {
            if(p->paren_depth == 0) {
                throw_fatal_error(2, "Unexpected ')'!");
            }
            p->paren_depth--;
        }
        
        return t;
    }

    int level = 0;
    if (t.type == T_INDENT) {
        level = token_get_indent_count(t);
        t = raw_scan(p);
    }

    // Skip empty lines and comments, even if they have no indentation
    if (t.type == T_NEWLINE) {
        return next_token(p);
    }
        

    if (p->paren_depth > 0) {
        if(level > p->logical_indent) {
            p->line_start = false;
            p->pending_token = t;
            p->has_pending_token = true;
            return next_token(p);
        }
        throw_fatal_error(2, "Wrong indentation inside brackets!");
    }

    // A logical line cannot start with a operator
    // Other operators are already rejected as statement starters
    // '-' or '!' need this check because it can start a valid unary expression
    if (t.type == T_OP_MINUS || t.type == T_OP_NOT) {
        throw_fatal_error(2, "Logical line cannot start with an operator!");
    }

    p->logical_indent = level;

    if (t.type == T_EOF) { 
        level = 0; 
        p->eof_newline_done = true; 
    }

    p->line_start = false; 
    p->pending_token = t; 
    p->has_pending_token = true;

    if (level > p->indent_stack[p->indent_count - 1]) {
        push_indent(p, level);
        return make_token(T_INDENT);
    }

    // Keep the base indentation 0 on the stack when removing levels
    while (p->indent_count > 1 && level < p->indent_stack[p->indent_count - 1]) { 
        p->indent_count--; 
        p->pending_dedent++; 
    }

    if (level != p->indent_stack[p->indent_count - 1]) {
        syntax_error("Inconsistent indentation");
    }

    return next_token(p);
}

void advance(Parser *p) {
    token_free_data_if_applicable(p->token);
    if(p->has_peek) {
        p->token = p->peek_token;
        p->has_peek = false;
    } else {
        p->token = next_token(p);                   
    }
}

void expect(Parser *p, TokenType type, char *msg) {
    if (p->token.type != type)
        throw_fatal_error(2, msg);
    advance(p);
}

TokenType peek_token_type(Parser *p) {
    if(!p->has_peek) {
        p->peek_token = next_token(p);
        p->has_peek = true;
    }
    
    return p->peek_token.type;
}

static void parse_prolog(Parser *p);

static void parse_function_list(Parser *p, ASTNode *program);

ASTNode *parse_function_def(Parser *p);

static bool parse_annotation_opt(Parser *p);

static void parse_annotation_remaining(Parser *p);

DataType parse_return_type(Parser *p);

DataType parse_type(Parser *p);

static void parse_params(Parser *p, ASTNode *func);

ASTNode *parse_one_param(Parser *p);

static void parse_param_tail(Parser *p, ASTNode *func);

static void parse_next_param(Parser *p, ASTNode *func);

ASTNode *parse_function_body(Parser *p);

ASTNode *parse_block(Parser *p);

static void parse_stmt_list(Parser *p, ASTNode *block);

static void parse_simple_end(Parser *p, ASTNode *block);

static void parse_stmt_list_end(Parser *p, ASTNode *block);

ASTNode *parse_simple_stmt(Parser *p);

ASTNode *parse_after_ident(Parser *p, char *name);

ASTNode *parse_return_val(Parser *p);

static bool parse_def_value(Parser *p);

ASTNode *parse_compound_stmt(Parser *p);

ASTNode *parse_if_stmt(Parser *p);

static void parse_else_stmt(Parser *p, ASTNode *if_node);

ASTNode *parse_else_body(Parser *p);



ASTNode *parse_program(FILE *file, SymTable *table) {
    Parser p;
    init_parser(&p, file, table);

    p.token = next_token(&p);     
    ASTNode *program = create_node(NODE_PROGRAM);     
    switch (p.token.type){
        case T_KW_IMPORT:
            parse_prolog(&p);
            parse_function_list(&p, program);
            expect(&p, T_EOF, "expected end of file after last function");
            break;
        default:
            throw_fatal_error(2, "Program must start with 'import ifj26.*'");
    }

    free(p.indent_stack);
    return program;
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
            throw_fatal_error(2, "Program must start with 'import'");
    }
}

static void parse_function_list(Parser *p, ASTNode *program){
    switch (p->token.type){
        case T_AT_MAIN:
        case T_KW_DEF:{
            ASTNode *func = parse_function_def(p);
            ast_array_append(&program->data.program.functions, &program->data.program.count, &program->data.program.capacity, func);
            parse_function_list(p, program);
            break;
        }
        case T_EOF:
            break;
        default:
            throw_fatal_error(2, "Expected function definition ('def' or '@main') or end of file");
    }   
}

ASTNode *parse_function_def(Parser *p){
    ASTNode *function_def = create_node(NODE_FUNCTION_DEF);
    switch (p->token.type){
        case T_AT_MAIN:
        case T_KW_DEF:
            function_def->data.function_def.is_main = parse_annotation_opt(p);
            expect(p, T_KW_DEF, "function definition: expected 'def'");

            if(p->token.type != T_IDENT){
                throw_fatal_error(2, "function definition: expected function name after 'def'");
            }
            function_def->data.function_def.name = p->token.data;
            p->token.data = NULL;
            advance(p);

            expect(p, T_PAR_L, "function definition: expected '(' after function name");
            parse_params(p, function_def);
            expect(p, T_PAR_R, "function definition: expected ')' after parameters");

            function_def->data.function_def.return_type = parse_return_type(p);
            function_def->data.function_def.has_explicit_return_type = (function_def->data.function_def.return_type != TYPE_UNKNOWN);
            expect(p, T_OP_ASSIGN, "function definition: expected '=' before function body");
            function_def->data.function_def.body = parse_function_body(p);
            break;
        default:
            throw_fatal_error(2, "Function definition: expected 'def' or '@main'");
    }
    return function_def;
}

static bool parse_annotation_opt(Parser *p){
    switch (p->token.type){
        case T_AT_MAIN:
            advance(p);
            parse_annotation_remaining(p);
            return true;
        case T_KW_DEF:
            return false;
        default:
            throw_fatal_error(2, "Expected '@main' or 'def'");
    }
    return false;
}

static void parse_annotation_remaining(Parser *p){
    switch (p->token.type){
        case T_NEWLINE:
            advance(p);
            break;
        case T_KW_DEF:
            break;
    default:
        throw_fatal_error(2, "After '@main': expected newline or 'def'");
    }
}

DataType parse_return_type(Parser *p){
    switch (p->token.type){
        case T_COLON:
            advance(p);
            return parse_type(p);
        case T_OP_ASSIGN:
            return TYPE_UNKNOWN;
    default:
        throw_fatal_error(2, "Function header: expected ':' with return type or '='");
    }
    return TYPE_UNKNOWN;
}

DataType parse_type (Parser *p){
    switch (p->token.type){
        case T_KW_INT:
            advance(p);
            return TYPE_INT;
        case T_KW_DOUBLE:
            advance(p);
            return TYPE_DOUBLE;
        case T_KW_STRING:
            advance(p);
            return TYPE_STRING;
        case T_KW_UNIT:
            advance(p);
            return TYPE_UNIT;
        case T_KW_BOOLEAN:
            advance(p);
            return TYPE_BOOL;
    default:
        throw_fatal_error(2, "Expected type (Int, Double, String, Unit or Boolean)");    
    }
    return TYPE_UNKNOWN;
}

static void parse_params(Parser *p, ASTNode *func){
    switch (p->token.type){
        case T_IDENT:{
            ASTNode *param = parse_one_param(p);
            ast_array_append(&func->data.function_def.parameters, &func->data.function_def.param_count, &func->data.function_def.capacity, param);
            parse_param_tail(p, func);
            break;
        }
        case T_PAR_R:
            break;
        default:
            throw_fatal_error(2, "Parameter list: expected parameter name or ')'");
    }
}

ASTNode *parse_one_param(Parser *p){
    switch (p->token.type){
        case T_IDENT:{
            ASTNode *param = create_node(NODE_PARAM);
            param->data.param.name = p->token.data;
            p->token.data = NULL;
            advance(p);
            expect(p, T_COLON, "parameter: expected ':' after parameter name");
            param->data.param.data = parse_type(p);
            return param;
        }
        default:
            throw_fatal_error(2, "Parameter: expected parameter name");
    }
    return NULL;
}

static void parse_param_tail(Parser *p, ASTNode *func){
    switch (p->token.type){
        case T_COMMA:{
            p->pending_lf = false;
            advance(p);
            parse_next_param(p, func);
            break;
        }    
        case T_PAR_R:
            break;
        default:
            throw_fatal_error(2, "Parameter list: expected ',' or ')'");
    } 
}

static void parse_next_param(Parser *p, ASTNode *func){
    switch (p->token.type){
        case T_IDENT:{
            ASTNode *param = parse_one_param(p);
            ast_array_append(&func->data.function_def.parameters, &func->data.function_def.param_count, &func->data.function_def.capacity, param);
            parse_param_tail(p, func);
            break;
        }
        case T_PAR_R:{
            if(!p->pending_lf){
                throw_fatal_error(2, "After last comma, ')' must be on a new line!");
            }
            break;
        }
        default:
            throw_fatal_error(2, "Parameter list: expected parameter name or ')' after ','");
    }
}

ASTNode *parse_function_body(Parser *p){
    switch (p->token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_OP_NOT:
        case T_KW_TRUE:
        case T_KW_FALSE:
        case T_PAR_L:{
            ASTNode *block = create_node(NODE_BLOCK);
            ast_array_append(&block->data.block.statements, &block->data.block.count, &block->data.block.capacity, parse_expression(p, EXPR_STATEMENT));
            expect(p, T_NEWLINE, "function body: expected newline after expression");
            return block;
        }    
        case T_NEWLINE:
            return parse_block(p);
        default:
            throw_fatal_error(2, "Function body: expected expression or newline with indented block");
    }
    return NULL;
}

ASTNode *parse_block(Parser *p){
    switch (p->token.type){
        case T_NEWLINE:{
            advance(p);
            expect(p, T_INDENT, "block: expected indented block");
            ASTNode *block = create_node(NODE_BLOCK);
            parse_stmt_list(p, block);
            return block;
        }
        default:
            throw_fatal_error(2, "Block: expected newline and indented block");
    }
    return NULL;
}

static void parse_stmt_list(Parser *p, ASTNode *block){
    switch (p->token.type){
        case T_KW_VAL:
        case T_KW_VAR:
        case T_KW_RETURN:
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_OP_NOT:
        case T_KW_TRUE:
        case T_KW_FALSE:
        case T_PAR_L:{   
            ASTNode *stmt = parse_simple_stmt(p);
            ast_array_append(&block->data.block.statements, &block->data.block.count, &block->data.block.capacity, stmt);
            parse_simple_end(p, block);
            break;
        }
        case T_KW_IF:
        case T_KW_WHILE:{
            ASTNode *stmt = parse_compound_stmt(p);
            ast_array_append(&block->data.block.statements, &block->data.block.count, &block->data.block.capacity, stmt);
            parse_stmt_list_end(p, block);
            break;
        }
        default:
            throw_fatal_error(2, "Block: expected statement");
    }
}

static void parse_simple_end(Parser *p, ASTNode *block){
    switch (p->token.type){
        case T_SEMICOLON:
            advance(p);
            parse_stmt_list(p, block);
            break;
        case T_NEWLINE:
            advance(p);
            parse_stmt_list_end(p, block);
            break;
        default:
            throw_fatal_error(2, "Statement: expected ';' or newline after statement");
    }
}

static void parse_stmt_list_end(Parser *p, ASTNode *block){
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
        case T_OP_NOT:
        case T_KW_TRUE:
        case T_KW_FALSE:
        case T_PAR_L:
            parse_stmt_list(p, block);
            break;
        case T_DEDENT:
            advance(p);
            break;
        default:
            throw_fatal_error(2, "Block: expected next statement or end of block");
    }
}

ASTNode *parse_simple_stmt(Parser *p){
    switch (p->token.type){
        case T_KW_VAL:
        case T_KW_VAR:{
            ASTNode *simple_value = create_node(NODE_DEF_VAR);
            simple_value->data.def_var.var = parse_def_value(p);
            if(p->token.type != T_IDENT){
                throw_fatal_error(2,"variable definition: expected variable name");
            }
            simple_value->data.def_var.name = p->token.data;
            p->token.data = NULL;
            advance(p);
            simple_value->data.def_var.type = parse_return_type(p);
            expect(p, T_OP_ASSIGN, "variable definition: expected '='");
            simple_value->data.def_var.value = parse_expression(p, EXPR_STATEMENT);
            return simple_value;
        }
        case T_IDENT:{ 
            if(peek_token_type(p) == T_OP_ASSIGN){
                char *name = p->token.data;
                p->token.data = NULL;
                advance(p);
                return parse_after_ident(p, name);
            }  
            return parse_expression(p, EXPR_STATEMENT);
        }
        case T_KW_RETURN:{
            advance(p);
            ASTNode *simple_return = create_node(NODE_RETURN);
            simple_return->data.return_stmt.value = parse_return_val(p);
            return simple_return;
        }
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_OP_NOT:
        case T_KW_TRUE:
        case T_KW_FALSE:
        case T_PAR_L:{
            return parse_expression(p, EXPR_STATEMENT);
        }
        default:
            throw_fatal_error(2, "Expected statement (val, var, return, assignment, call or expression)");
    }
    return NULL;
}

ASTNode *parse_after_ident(Parser *p, char *name){
    switch (p->token.type){
        case T_OP_ASSIGN:{
            advance(p);
            ASTNode *after_assign = create_node(NODE_ASSIGN);
            after_assign->data.assign.name = name;
            after_assign->data.assign.value = parse_expression(p, EXPR_STATEMENT);
            return after_assign;
        }
        default:
            throw_fatal_error(2, "After identifier: expected '=', '(', operator or end of statement");
    }
    return NULL;
}

ASTNode *parse_return_val(Parser *p){
    switch (p->token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_OP_NOT:
        case T_KW_TRUE:
        case T_KW_FALSE:
        case T_PAR_L:
            return parse_expression(p, EXPR_STATEMENT);
        case T_SEMICOLON:
        case T_NEWLINE:
            return NULL;
        default:
            throw_fatal_error(2, "Return: expected expression or end of statement");
    }
    return NULL;
}

static bool parse_def_value(Parser *p){
    switch (p->token.type){
        case T_KW_VAL:
            advance(p);
            return false;
        case T_KW_VAR:
            advance(p);
            return true;
        default:
            throw_fatal_error(2, "Variable definition: expected 'val' or 'var'");
    }
    return false;
}

ASTNode *parse_compound_stmt(Parser *p){
    switch (p->token.type){
        case T_KW_IF:{
            return parse_if_stmt(p);
        }
        case T_KW_WHILE:{
            advance(p);
            ASTNode *comp_while = create_node(NODE_WHILE);
            comp_while->data.while_loop.condition = parse_expression(p, EXPR_STATEMENT);
            expect(p, T_KW_DO, "while: expected 'do' after condition");
            comp_while->data.while_loop.body = parse_block(p);
            return comp_while;
        }
        default:
            throw_fatal_error(2, "Expected 'if' or 'while'");
    }
    return NULL;
}

ASTNode *parse_if_stmt(Parser *p){
    switch (p->token.type){
        case T_KW_IF:{
            advance(p);

            ASTNode *comp_if = create_node(NODE_IF);
            comp_if->data.if_stmt.condition = parse_expression(p, EXPR_STATEMENT);

            expect(p, T_KW_THEN, "if: expected 'then' after condition");
            comp_if->data.if_stmt.then_block = parse_block(p);
            parse_else_stmt(p, comp_if);
            return comp_if;
        }
        default:
            throw_fatal_error(2, "Expected 'if'");
    }
    return NULL;
}

static void parse_else_stmt(Parser *p, ASTNode *if_node){
     switch (p->token.type){
        case T_KW_ELSE:{
            advance(p);
            if_node->data.if_stmt.else_block = parse_else_body(p);
            break;
        }
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
        case T_OP_NOT:
        case T_KW_TRUE:
        case T_KW_FALSE:
        case T_PAR_L:
        case T_DEDENT:{
            break;
        }
        default:
            throw_fatal_error(2, "After if block: expected 'else', next statement or end of block");
     }
}

ASTNode *parse_else_body(Parser *p){
    switch(p->token.type){
        case T_NEWLINE:{
            return parse_block(p);
        }
        case T_KW_IF:{
            ASTNode *else_block = create_node(NODE_BLOCK);
            ast_array_append(&else_block->data.block.statements, &else_block->data.block.count, &else_block->data.block.capacity, parse_if_stmt(p));
            return else_block;
        }
        default:
            throw_fatal_error(2, "After 'else': expected newline with block or 'if'");
    }
    return NULL;    
}