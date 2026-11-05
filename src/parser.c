#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "scanner.h"
#include "token.h"
#include "library.h"
#include "parser.h"
#include "ast.h"

#define T_DEDENT ((TokenType)(T_EOF + 1))

static void syntax_error(const char *msg) {
    fprintf(stderr, "Syntax error: %s\n", msg);
    exit(2);
}


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

void advance(Parser *p) {
    token_free_data_if_applicable(p->token);   
    p->token = next_token(p);                   
}

static void expect(Parser *p, TokenType type, char *msg) {
    if (p->token.type != type)
        throw_fatal_error(2, msg);
    advance(p);
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

ASTNode *parse_function_body(Parser *p);

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

ASTNode *parse_block(Parser *p);

static void parse_stmt_list(Parser *p, ASTNode *block);

static void parse_simple_end(Parser *p, ASTNode *block);

static void parse_stmt_list_end(Parser *p, ASTNode *block);

ASTNode *parse_simple_stmt(Parser *p);

ASTNode *parse_after_ident(Parser *p, char *name);

ASTNode *parse_return_val(Parser *p);

static bool parse_def_value(Parser *p);

ASTNode *parse_compound_stmt(Parser *p);

static void parse_cond(Parser *p);


ASTNode *parse_program(FILE *file, SymTable *table) {
    Parser p = { 
        .src = file,
        .symtab = table, 
        .last_scanned = T_NEWLINE
    };
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
    default:
        throw_fatal_error(2, "Expected type (Int, Double, String or Unit)");    
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
            advance(p);
            ASTNode *param = parse_one_param(p);
            ast_array_append(&func->data.function_def.parameters, &func->data.function_def.param_count, &func->data.function_def.capacity, param);
            parse_param_tail(p, func);
            break;
        }    
        case T_PAR_R:
            break;
        default:
            throw_fatal_error(2, "Parameter list: expected ',' or ')'");
    } 
}

ASTNode *parse_function_body(Parser *p){
    switch (p->token.type){
        case T_IDENT:
        case T_INT:
        case T_DOUBLE:
        case T_STRING:
        case T_OP_MINUS:
        case T_PAR_L:{
            ASTNode *block = create_node(NODE_BLOCK);
            ASTNode *ret = create_node(NODE_RETURN);
            parse_single_line_body(p);
            ret->data.return_stmt.value = NULL;
            ast_array_append(&block->data.block.statements, &block->data.block.count, &block->data.block.capacity, ret);
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
            throw_fatal_error(2, "Expected expression or function call");
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
            throw_fatal_error(2, "After identifier: expected '(', operator or end of statement");
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
            throw_fatal_error(2, "function call: expected argument or ')'");
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
            throw_fatal_error(2, "Expression: expected operator or end of expression");
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
            throw_fatal_error(2, "Expression: expected binary operator");
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
            throw_fatal_error(2, "Expression: expected operand (identifier, literal, '-' or '(')");
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
            throw_fatal_error(2, "Expected literal (integer, double or string)");
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
            throw_fatal_error(2, "Function call: expected argument (identifier or literal)");
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
            throw_fatal_error(2, "Function call: argument must be identifier or literal");
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
            throw_fatal_error(2, "Function call: expected ',' or ')'");
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
            throw_fatal_error(2, "Expression: expected literal, '-' or '('");
    }
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
            parse_single_line_body(p);
            simple_value->data.def_var.value = NULL;
            return simple_value;
        }
        case T_IDENT:{   
            char *name = p->token.data;
            p->token.data = NULL;
            advance(p);
            return parse_after_ident(p, name);
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
        case T_PAR_L:{
            parse_non_ident_expr_start(p);
            return NULL;
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
            parse_single_line_body(p);
            after_assign->data.assign.value = NULL;
            return after_assign;
        }
        case T_PAR_L:{
            ASTNode *call = create_node(NODE_FUNCTION_CALL);
            call->data.function_call.name = name;
            advance(p);
            parse_args(p);
            expect(p, T_PAR_R, "function call: expected ')' after arguments");
            return call;
        }
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
        case T_NEWLINE:{
            parse_ident_suffix(p);
            free(name);
            return NULL;
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
        case T_PAR_L:
            parse_single_line_body(p);
            return NULL;
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
        case T_KW_IF:
            advance(p);
            ASTNode *comp_if = create_node(NODE_IF);
            parse_cond(p);
            comp_if->data.if_stmt.condition = NULL;
            expect(p, T_KW_THEN, "if: expected 'then' after condition");
            comp_if->data.if_stmt.then_block = parse_block(p);
            expect(p, T_KW_ELSE, "if: expected 'else'");
            comp_if->data.if_stmt.else_block = parse_block(p);
            return comp_if;
        case T_KW_WHILE:
            advance(p);
            ASTNode *comp_while = create_node(NODE_WHILE);
            parse_cond(p);
            comp_while->data.while_loop.condition = NULL;
            expect(p, T_KW_DO, "while: expected 'do' after condition");
            comp_while->data.while_loop.body = parse_block(p);
            return comp_while;
        default:
            throw_fatal_error(2, "Expected 'if' or 'while'");
    }
    return NULL;
}

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
            throw_fatal_error(2, "Condition: expected expression");
    }
}