/**
 * @file parser.c
 * @brief Syntax analysis of IFJ26 (LL(1)).
 *
 * The parser reads tokens and checks that the program follows the grammar.
 * It uses recursive descent: every non-terminal has its own function parse_<name>.
 * Rule numbers match LL_Table.xlsx (rules 1-62, numbered in the order of the functions
 * in this file, extension rules LAMBDAS and EXTKW included).
 * Expressions are parsed by parse_expression (precedence parser).
 * While parsing, the parser builds the AST.
 *
 * next_token is a small layer between the scanner and parser.
 * It makes INDENT and DEDENT tokens from the indentation, skips empty lines,
 * ignores newlines inside brackets and does not allow '-' or '!' at the start of a line.
 *
 * @author Jakub Juřík (xjurikj00)
 * @date 4.10.2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "expression_parser.h"
#include "library.h"
#include "parser.h"
#include "scanner.h"
#include "symtable.h"
#include "token.h"

/**
 * @brief Prints a syntax error and ends the program with code 2.
 *
 * @param msg Error message.
 */
static void syntax_error(const char *msg) {
    fprintf(stderr, "Syntax error: %s\n", msg);
    exit(2);
}

/**
 * @brief Initialize parser - sets to its start state.
 *
 * @param p Parser
 * @param file Input file
 * @param table Symbol table
 */
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
    p->ended_with_block = false;

    p->indent_stack = (int *)malloc(p->indent_capacity * sizeof(int));
    if (p->indent_stack == NULL) {
        throw_fatal_error(99, "Could not allocate memory!");
    }

    p->indent_stack[p->indent_count++] = 0;
}

/**
 * @brief Adds a new indentation level to the stack.
 *
 * Makes the stack bigger when it's full.
 *
 * @param p Parser
 * @param level Number of spaces of the new block
 */
static void push_indent(Parser *p, int level) {
    if (p->indent_count == p->indent_capacity) {
        size_t new_capacity = p->indent_capacity * 2;
        int *new_stack = realloc(p->indent_stack, new_capacity * sizeof(int));
        if (new_stack == NULL) {
            throw_fatal_error(99, "Could not allocate memory!");
        }
        p->indent_stack = new_stack;
        p->indent_capacity = new_capacity;
    }

    p->indent_stack[p->indent_count++] = level;
}

/**
 * @brief Makes a new token without data.
 *
 * Used for tokens made by parser (INDENT, DEDENT, NEWLINE).
 *
 * @param type Token type
 *
 * @return New token
 */
static Token make_token(TokenType type) {
    Token t = {type, NULL};
    return t;
}

/**
 * @brief Gets the new token from the scanner.
 *
 * If a token was saved for late, it returns that token first.
 *
 * @param p Parser
 *
 * @return Next token from the scanner
 */
static Token raw_scan(Parser *p) {
    if (p->has_pending_token) {
        p->has_pending_token = false;
        return p->pending_token;
    }
    Token t = scanner(p->src);
    return t;
}

/**
 * @brief Gets the next token for the parser.
 *
 * - makes INDENT/DEDENT tokens from the indentation,
 * - skips empty lines and lines with only comments,
 * - inside brackets a newline is only a space, the next line must be indented more than the start
 * of the statement,
 * - a line cannot start with '-' or '!',
 * - adds NEWLINE before EOF if it is missing.
 *
 * @param p Parser
 *
 * @return Next token
 */
static Token next_token(Parser *p) {
    if (p->pending_dedent > 0) {
        p->pending_dedent--;
        return make_token(T_DEDENT);
    }
    Token t = raw_scan(p);

    if (!p->line_start) {
        if (t.type == T_NEWLINE) {
            p->line_start = true;

            if (p->paren_depth > 0) {
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
            if (p->paren_depth == 0) {
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
        if (level > p->logical_indent) {
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
    if (p->has_peek) {
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
    if (!p->has_peek) {
        p->peek_token = next_token(p);
        p->has_peek = true;
    }

    return p->peek_token.type;
}

static void parse_prolog(Parser *p);

static void parse_function_list(Parser *p, ASTNode *program);

static const char *receiver_type_name(DataType type);

static void parse_extension(Parser *p, ASTNode *program);

static ASTNode *parse_extension_method(Parser *p, ASTNode *receiver);

ASTNode *parse_function_def(Parser *p);

static bool parse_annotation_opt(Parser *p);

static void parse_annotation_remaining(Parser *p);

DataType parse_return_type(Parser *p, FunType **fun);

DataType parse_type(Parser *p, FunType **fun);

void parse_params(Parser *p, ASTNode *func);

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

// Parses a whole program and returns its AST (see parser.h)
// Rule 1: PROGRAM -> PROLOG FUNCTION_LIST
ASTNode *parse_program(FILE *file, SymTable *table) {
    Parser p;
    init_parser(&p, file, table);

    p.token = next_token(&p);
    ASTNode *program = create_node(NODE_PROGRAM);
    p.program = program;
    switch (p.token.type) {
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

/**
 * @brief Parses the first line "import ifj26.*".
 *
 * Rule 2: PROLOG -> T_KW_IMPORT T_IDENT T_DOT T_OP_MUL T_NEWLINE
 *
 * @param p Parser
 */
static void parse_prolog(Parser *p) {
    switch (p->token.type) {
    case T_KW_IMPORT:
        advance(p);
        if ((p->token.type != T_IDENT) || (strcmp((char *)p->token.data, "ifj26") != 0)) {
            syntax_error("prolog: expected 'ifj26'");
        }
        advance(p);
        expect(p, T_DOT, "prolog: expected '.'");
        expect(p, T_OP_MUL, "prolog: expected '*'");
        expect(p, T_NEWLINE, "prolog: expected 'import ifj26.*'");
        break;
    default:
        throw_fatal_error(2, "Program must start with 'import'");
    }
}

/**
 * @brief Parses all functions and adds them to the program.
 *
 * Rule 3: FUNCTION_LIST -> FUNCTION_DEF FUNCTION_LIST
 * Rule 4: FUNCTION_LIST -> EXTENSION FUNCTION_LIST                    (EXTKW)
 * Rule 5: FUNCTION_LIST -> ε
 *
 * @param p Parser
 * @param program Program node
 */
static void parse_function_list(Parser *p, ASTNode *program) {
    switch (p->token.type) {
    case T_AT_MAIN:
    case T_KW_DEF: {
        ASTNode *func = parse_function_def(p);
        ast_array_append(&program->data.program.functions, &program->data.program.count,
                         &program->data.program.capacity, func);
        parse_function_list(p, program);
        break;
    }
    case T_EOF:
        break;
    // extension (value: Type) with methods (EXTKW)
    case T_KW_EXTENSION: {
        parse_extension(p, program);
        parse_function_list(p, program);
        break;
    }
    default:
        throw_fatal_error(
            2, "Expected function definition ('def', '@main' or 'extension') or end of file");
    }
}

/**
 * @brief Gives the name of a type, used in names of extension methods.
 *
 * @param type Type of the receiver
 *
 * @return Name of the type (Int, Double, String, Boolean, Unit)
 */
static const char *receiver_type_name(DataType type) {
    switch (type) {
    case TYPE_INT:
        return "Int";
    case TYPE_DOUBLE:
        return "Double";
    case TYPE_STRING:
        return "String";
    case TYPE_BOOL:
        return "Boolean";
    case TYPE_UNIT:
        return "Unit";
    default:
        throw_fatal_error(2, "extension: unsupported receiver type");
    }
    return "";
}

/**
 * @brief Parses an extension block with its methods (EXTKW).
 *
 * Rule 6: EXTENSION -> T_KW_EXTENSION T_PAR_L ONE_PARAM T_PAR_R T_NEWLINE T_INDENT
 *                     METHOD METHOD_LIST T_DEDENT
 * Rule 7: METHOD_LIST -> METHOD METHOD_LIST
 * Rule 8: METHOD_LIST -> ε
 *
 * Every method is saved as a normal function "<Type>$<method>" with the receiver
 * as its first parameter, e.g. "def twice: Int" on Int becomes Int$twice(value: Int).
 *
 * @param p Parser
 * @param program Program node (the methods are added to its functions)
 */
static void parse_extension(Parser *p, ASTNode *program) {
    switch (p->token.type) {
    case T_KW_EXTENSION: {
        expect(p, T_KW_EXTENSION, "extension: expected 'extension'");
        expect(p, T_PAR_L, "extension: expected '(' before the receiver");
        ASTNode *receiver = parse_one_param(p);
        expect(p, T_PAR_R, "extension: expected ')' after the receiver");
        expect(p, T_NEWLINE, "extension: expected newline after the receiver");
        expect(p, T_INDENT, "extension: expected indented methods");
        do {
            ASTNode *method = parse_extension_method(p, receiver);
            ast_array_append(&program->data.program.functions, &program->data.program.count,
                             &program->data.program.capacity, method);
        } while (p->token.type == T_KW_DEF);
        expect(p, T_DEDENT, "extension: expected 'def' or end of the extension block");
        free_ast(receiver);
        break;
    }
    default:
        throw_fatal_error(2, "Expected 'extension'");
    }
}

/**
 * @brief Parses one method inside an extension block (EXTKW).
 *
 * Rule 9: METHOD -> T_KW_DEF T_IDENT METHOD_PARAMS RETURN_TYPE T_OP_ASSIGN FUNCTION_BODY
 * Rule 10: METHOD_PARAMS -> T_PAR_L PARAMS T_PAR_R   (called with parentheses, also for "()")
 * Rule 11: METHOD_PARAMS -> ε                         (paramless method, called without "()")
 *
 * @param p Parser
 * @param receiver Receiver parameter of the extension block
 *
 * @return Function node "<Type>$<name>" with the receiver as the first parameter
 */
static ASTNode *parse_extension_method(Parser *p, ASTNode *receiver) {
    switch (p->token.type) {
    case T_KW_DEF: {
        expect(p, T_KW_DEF, "extension: expected 'def'");
        if (p->token.type != T_IDENT) {
            throw_fatal_error(2, "extension: expected method name after 'def'");
        }

        // Function node for the method
        ASTNode *func = create_node(NODE_FUNCTION_DEF);
        func->data.function_def.is_extension = true;

        // Name "<Type>$<method>", '$' cannot be in a user name
        const char *type_name = receiver_type_name(receiver->data.param.data);
        char *method_name = p->token.data;
        size_t len = (strlen(type_name) + 1) + (strlen(method_name) + 1);
        char *name = malloc(len);
        if (name == NULL) {
            throw_fatal_error(99, "Allocation error");
        }
        snprintf(name, len, "%s$%s", type_name, method_name);
        func->data.function_def.name = name;
        advance(p);

        // Receiver as a first parameter
        ASTNode *recv = create_node(NODE_PARAM);
        recv->data.param.name = strdup(receiver->data.param.name);
        if (recv->data.param.name == NULL) {
            throw_fatal_error(99, "Allocation error");
        }
        recv->data.param.data = receiver->data.param.data;
        ast_array_append(&func->data.function_def.parameters, &func->data.function_def.param_count,
                         &func->data.function_def.capacity, recv);

        // Parameters
        if (p->token.type == T_PAR_L) {
            advance(p);
            parse_params(p, func);
            expect(p, T_PAR_R, "extension: expected ')' after parameters");
        } else {
            func->data.function_def.is_paramless = true;
        }

        // Return type (optional) and body, the same as in parse_function_def
        func->data.function_def.return_type =
            parse_return_type(p, &func->data.function_def.return_fun);
        func->data.function_def.has_explicit_return_type =
            (func->data.function_def.return_type != TYPE_UNKNOWN);
        expect(p, T_OP_ASSIGN, "extension: expected '=' before method body");
        func->data.function_def.body = parse_function_body(p);
        return func;
    }
    default:
        throw_fatal_error(2, "extension: expected 'def'");
    }
    return NULL;
}

/**
 * @brief Parses one function.
 *
 * Rule 12: FUNCTION_DEF -> ANNOTATION_OPT T_KW_DEF T_IDENT T_PAR_L PARAMS T_PAR_R RETURN_TYPE
 * T_OP_ASSIGN FUNCTION_BODY
 *
 * @param p Parser
 *
 * @return Function node (NODE_FUNCTION_DEF)
 */
ASTNode *parse_function_def(Parser *p) {
    ASTNode *function_def = create_node(NODE_FUNCTION_DEF);
    switch (p->token.type) {
    case T_AT_MAIN:
    case T_KW_DEF:
        function_def->data.function_def.is_main = parse_annotation_opt(p);
        expect(p, T_KW_DEF, "function definition: expected 'def'");

        if (p->token.type != T_IDENT) {
            throw_fatal_error(2, "function definition: expected function name after 'def'");
        }
        function_def->data.function_def.name = p->token.data;
        p->token.data = NULL;
        advance(p);

        expect(p, T_PAR_L, "function definition: expected '(' after function name");
        parse_params(p, function_def);
        expect(p, T_PAR_R, "function definition: expected ')' after parameters");

        function_def->data.function_def.return_type =
            parse_return_type(p, &function_def->data.function_def.return_fun);
        function_def->data.function_def.has_explicit_return_type =
            (function_def->data.function_def.return_type != TYPE_UNKNOWN);
        expect(p, T_OP_ASSIGN, "function definition: expected '=' before function body");
        function_def->data.function_def.body = parse_function_body(p);
        break;
    default:
        throw_fatal_error(2, "Function definition: expected 'def' or '@main'");
    }
    return function_def;
}

/**
 * @brief Parses the optional @main before def.
 *
 * Rule 13: ANNOTATION_OPT -> T_AT_MAIN ANNOTATION_REMAINING
 * Rule 14: ANNOTATION_OPT -> ε
 *
 * @param p Parser
 *
 * @return True if the function has @main
 */
static bool parse_annotation_opt(Parser *p) {
    switch (p->token.type) {
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

/**
 * @brief Parses the optional new line after @main.
 *
 * Rule 15: ANNOTATION_REMAINING -> T_NEWLINE
 * Rule 16: ANNOTATION_REMAINING -> ε
 *
 * @param p Parser
 */
static void parse_annotation_remaining(Parser *p) {
    switch (p->token.type) {
    case T_NEWLINE:
        advance(p);
        break;
    case T_KW_DEF:
        break;
    default:
        throw_fatal_error(2, "After '@main': expected newline or 'def'");
    }
}

/**
 * @brief Parses the optional type ": Type".
 *
 * Rule 17: RETURN_TYPE -> T_COLON TYPE
 * Rule 18: RETURN_TYPE -> ε
 *
 * @param p Parser
 * @param fun Output: the function type if the type is a function type, otherwise NULL
 *
 * @return The type or TYPE_UNKNOWN if there is no type
 */
DataType parse_return_type(Parser *p, FunType **fun) {
    switch (p->token.type) {
    case T_COLON:
        advance(p);
        return parse_type(p, fun);
    case T_OP_ASSIGN:
        *fun = NULL;
        return TYPE_UNKNOWN;
    default:
        throw_fatal_error(2, "Function header: expected ':' with return type or '='");
    }
    return TYPE_UNKNOWN;
}

/**
 * @brief Parses a type name or a function type.
 *
 * Rule 19: TYPE -> T_KW_INT
 * Rule 20: TYPE -> T_KW_DOUBLE
 * Rule 21: TYPE -> T_KW_STRING
 * Rule 22: TYPE -> T_KW_UNIT
 * Rule 23: TYPE -> T_KW_BOOLEAN
 * Rule 24: TYPE -> T_PAR_L TYPE_LIST T_PAR_R T_OP_ROCKET TYPE        (LAMBDAS)
 * Rule 25: TYPE_LIST -> TYPE TYPE_LIST_TAIL
 * Rule 26: TYPE_LIST -> ε
 * Rule 27: TYPE_LIST_TAIL -> T_COMMA TYPE TYPE_LIST_TAIL
 * Rule 28: TYPE_LIST_TAIL -> ε
 * (TYPE_LIST and TYPE_LIST_TAIL are done by the loop in the T_PAR_L case.)
 *
 * A function type can have function types inside it,
 * e.g. (Int) => (Int) => Int, so it is parsed recursively.
 *
 * @param p Parser
 * @param fun Output: new function type if the type is TYPE_FUNCTION
 *            (the caller owns it), otherwise NULL
 *
 * @return Type (TYPE_FUNCTION for a function type)
 */
DataType parse_type(Parser *p, FunType **fun) {
    switch (p->token.type) {
    case T_KW_INT:
        advance(p);
        *fun = NULL;
        return TYPE_INT;
    case T_KW_DOUBLE:
        advance(p);
        *fun = NULL;
        return TYPE_DOUBLE;
    case T_KW_STRING:
        advance(p);
        *fun = NULL;
        return TYPE_STRING;
    case T_KW_UNIT:
        advance(p);
        *fun = NULL;
        return TYPE_UNIT;
    case T_KW_BOOLEAN:
        advance(p);
        *fun = NULL;
        return TYPE_BOOL;
    case T_IDENT:
        throw_fatal_error(3, "Undefined type");
        break;
    case T_PAR_L: {
        advance(p);
        TypeRef *params = NULL;
        size_t count = 0;
        size_t capacity = 0;
        if (p->token.type != T_PAR_R) {
            while (true) {
                if (count == capacity) {
                    size_t new_capacity;
                    if (capacity == 0) {
                        new_capacity = 4;
                    } else {
                        new_capacity = capacity * 2;
                    }
                    TypeRef *array = realloc(params, new_capacity * sizeof(TypeRef));
                    if (array == NULL) {
                        throw_fatal_error(99, "reallocation error");
                    }
                    params = array;
                    capacity = new_capacity;
                }
                FunType *param_fun = NULL;
                params[count].kind = parse_type(p, &param_fun);
                params[count].fun = param_fun;
                count++;

                if (p->token.type == T_COMMA) {
                    advance(p);
                    continue;
                } else {
                    break;
                }
            }
        }
        expect(p, T_PAR_R, "Function type: expected ')' after parameter types");
        expect(p, T_OP_ROCKET, "Function type: expected '=>' after ')'");
        FunType *ret_fun = NULL;
        DataType ret_kind = parse_type(p, &ret_fun);
        FunType *result = create_fun_type(count);
        for (size_t i = 0; i < count; i++) {
            result->params[i] = params[i];
        }
        result->ret.kind = ret_kind;
        result->ret.fun = ret_fun;
        free(params);
        *fun = result;
        return TYPE_FUNCTION;
    }
    default:
        throw_fatal_error(2, "Expected type (Int, Double, String, Unit or Boolean)");
    }
    return TYPE_UNKNOWN;
}

/**
 * @brief Parses the parameters of a function (can be empty).
 *
 * Rule 29: PARAMS -> ONE_PARAM PARAM_TAIL
 * Rule 30: PARAMS -> ε
 *
 * @param p Parser
 * @param func Function node (the parameters are added to it)
 */
void parse_params(Parser *p, ASTNode *func) {
    switch (p->token.type) {
    case T_IDENT: {
        ASTNode *param = parse_one_param(p);
        ast_array_append(&func->data.function_def.parameters, &func->data.function_def.param_count,
                         &func->data.function_def.capacity, param);
        parse_param_tail(p, func);
        break;
    }
    case T_PAR_R:
        break;
    default:
        throw_fatal_error(2, "Parameter list: expected parameter name or ')'");
    }
}

/**
 * @brief Parses one parameter "name: Type".
 *
 * Rule 31: ONE_PARAM -> T_IDENT T_COLON TYPE
 *
 * @param p Parser
 *
 * @return Parameter node (NODE_PARAM)
 */
ASTNode *parse_one_param(Parser *p) {
    switch (p->token.type) {
    case T_IDENT: {
        ASTNode *param = create_node(NODE_PARAM);
        param->data.param.name = p->token.data;
        p->token.data = NULL;
        advance(p);
        expect(p, T_COLON, "parameter: expected ':' after parameter name");
        param->data.param.data = parse_type(p, &param->data.param.fun);
        return param;
    }
    default:
        throw_fatal_error(2, "Parameter: expected parameter name");
    }
    return NULL;
}

/**
 * @brief Parses rest of the parameters (after one_param).
 *
 * Rule 32: PARAM_TAIL -> T_COMMA NEXT_PARAM
 * Rule 33: PARAM_TAIL -> ε
 *
 * @param p Parser
 * @param func Function node
 */
static void parse_param_tail(Parser *p, ASTNode *func) {
    switch (p->token.type) {
    case T_COMMA: {
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

/**
 * @brief Parses what comes after a comma in the parameters.
 *
 * A comma at the and is allowed only if  ')' is on a new line.
 *
 * Rule 34: NEXT_PARAM -> ONE_PARAM PARAM_TAIL
 * Rule 35: NEXT_PARAM -> ε
 *
 * @param p Parser
 * @param func Function node
 */
static void parse_next_param(Parser *p, ASTNode *func) {
    switch (p->token.type) {
    case T_IDENT: {
        ASTNode *param = parse_one_param(p);
        ast_array_append(&func->data.function_def.parameters, &func->data.function_def.param_count,
                         &func->data.function_def.capacity, param);
        parse_param_tail(p, func);
        break;
    }
    case T_PAR_R: {
        if (!p->pending_lf) {
            throw_fatal_error(2, "After last comma, ')' must be on a new line!");
        }
        break;
    }
    default:
        throw_fatal_error(2, "Parameter list: expected parameter name or ')' after ','");
    }
}

/**
 * @brief Parses the body of a function.
 *
 * A one-line body is saved as a block with one expression, so both kinds of body look same in the
 * AST.
 *
 * Rule 36: FUNCTION_BODY -> EXPR T_NEWLINE
 * Rule 37: FUNCTION_BODY -> BLOCK
 *
 * @param p Parser
 *
 * @return Block node (NODE_BLOCK)
 */
ASTNode *parse_function_body(Parser *p) {
    switch (p->token.type) {
    case T_IDENT:
    case T_INT:
    case T_DOUBLE:
    case T_STRING:
    case T_OP_MINUS:
    case T_OP_NOT:
    case T_KW_TRUE:
    case T_KW_FALSE:
    case T_PAR_L: {
        ASTNode *block = create_node(NODE_BLOCK);
        ast_array_append(&block->data.block.statements, &block->data.block.count,
                         &block->data.block.capacity, parse_expression(p, EXPR_STATEMENT));
        if (p->ended_with_block) {
            p->ended_with_block = false;
            return block;
        }
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

/**
 * @brief Parses an indented block of statements.
 *
 * Rule 38: BLOCK -> T_NEWLINE T_INDENT STMT_LIST
 *
 * @param p Parser
 *
 * @return Block node (NODE_BLOCK)
 */
ASTNode *parse_block(Parser *p) {
    switch (p->token.type) {
    case T_NEWLINE: {
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

/**
 * @brief Parses one statement and then the rest of the block.
 *
 * Rule 39: STMT_LIST -> SIMPLE_STMT SIMPLE_END
 * Rule 40: STMT_LIST -> COMPOUND_STMT STMT_LIST_END
 *
 * @param p Parser
 * @param block Block node
 */
static void parse_stmt_list(Parser *p, ASTNode *block) {
    switch (p->token.type) {
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
    case T_PAR_L: {
        ASTNode *stmt = parse_simple_stmt(p);
        ast_array_append(&block->data.block.statements, &block->data.block.count,
                         &block->data.block.capacity, stmt);
        parse_simple_end(p, block);
        break;
    }
    case T_KW_IF:
    case T_KW_WHILE: {
        ASTNode *stmt = parse_compound_stmt(p);
        ast_array_append(&block->data.block.statements, &block->data.block.count,
                         &block->data.block.capacity, stmt);
        parse_stmt_list_end(p, block);
        break;
    }
    default:
        throw_fatal_error(2, "Block: expected statement");
    }
}

/**
 * @brief Parses ";" or a new line after a simple statement.
 *
 * If the statement ended with a lambda block body (p->ended_with_block),
 * there is no ';' or newline, the next statement or end of block follows directly.
 *
 * Rule 41: SIMPLE_END -> T_SEMICOLON STMT_LIST
 * Rule 42: SIMPLE_END -> T_NEWLINE STMT_LIST_END
 *
 * @param p Parser
 * @param block Block node
 */
static void parse_simple_end(Parser *p, ASTNode *block) {
    if (p->ended_with_block) {
        p->ended_with_block = false;
        parse_stmt_list_end(p, block);
        return;
    }
    switch (p->token.type) {
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

/**
 * @brief Parses the next statement or end of the block.
 *
 * Rule 43: STMT_LIST_END -> STMT_LIST
 * Rule 44: STMT_LIST_END -> T_DEDENT
 *
 * @param p Parser
 * @param block Block node
 */
static void parse_stmt_list_end(Parser *p, ASTNode *block) {
    switch (p->token.type) {
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

/**
 * @brief Parses a simple statement.
 *
 * Rule 45: SIMPLE_STMT -> DEF_VALUE T_IDENT RETURN_TYPE T_OP_ASSIGN VALUE_INIT
 * Rule 46: SIMPLE_STMT -> T_IDENT AFTER_IDENT
 * Rule 47: SIMPLE_STMT -> T_KW_RETURN RETURN_VAL
 * Rule 48: SIMPLE_STMT -> EXPR
 * Rule 49: VALUE_INIT -> EXPR
 * Rule 50: VALUE_INIT -> T_NEWLINE T_INDENT EXPR T_NEWLINE T_DEDENT      (LAMBDAS)
 *
 * A newline after '=' is allowed only when a lambda follows on the next
 * (indented) line. If the lambda has a block body, its block already ends
 * the line, so T_NEWLINE is not expected there.
 *
 * @param p Parser
 *
 * @return Statement node
 */
ASTNode *parse_simple_stmt(Parser *p) {
    switch (p->token.type) {
    case T_KW_VAL:
    case T_KW_VAR: {
        ASTNode *simple_value = create_node(NODE_DEF_VAR);
        simple_value->data.def_var.var = parse_def_value(p);
        if (p->token.type != T_IDENT) {
            throw_fatal_error(2, "variable definition: expected variable name");
        }
        simple_value->data.def_var.name = p->token.data;
        p->token.data = NULL;
        advance(p);
        simple_value->data.def_var.type =
            parse_return_type(p, &simple_value->data.def_var.type_fun);
        expect(p, T_OP_ASSIGN, "variable definition: expected '='");
        if (p->token.type == T_NEWLINE) {
            advance(p);
            expect(p, T_INDENT, "variable definition: expected indented value after '='");

            simple_value->data.def_var.value = parse_expression(p, EXPR_STATEMENT);
            if (simple_value->data.def_var.value->type != NODE_LAMBDA) {
                throw_fatal_error(
                    2, "variable definition: newline after '=' is allowed only before a lambda");
            }

            if (p->ended_with_block) {
                p->ended_with_block = false;
            } else {
                expect(p, T_NEWLINE, "variable definition: expected newline after lambda");
            }

            expect(p, T_DEDENT, "variable definition: expected end of indented value after lambda");
            p->ended_with_block = true;
        } else {
            simple_value->data.def_var.value = parse_expression(p, EXPR_STATEMENT);
        }

        return simple_value;
    }
    // id followed by '=' is an assignment, otherwise an expression
    case T_IDENT: {
        if (peek_token_type(p) == T_OP_ASSIGN) {
            char *name = p->token.data;
            p->token.data = NULL;
            advance(p);
            return parse_after_ident(p, name);
        }
        return parse_expression(p, EXPR_STATEMENT);
    }
    case T_KW_RETURN: {
        advance(p);
        ASTNode *simple_return = create_node(NODE_RETURN);
        simple_return->data.return_stmt.value = parse_return_val(p);
        return simple_return;
    }
    // Expression or function call
    case T_INT:
    case T_DOUBLE:
    case T_STRING:
    case T_OP_MINUS:
    case T_OP_NOT:
    case T_KW_TRUE:
    case T_KW_FALSE:
    case T_PAR_L: {
        return parse_expression(p, EXPR_STATEMENT);
    }
    default:
        throw_fatal_error(2,
                          "Expected statement (val, var, return, assignment, call or expression)");
    }
    return NULL;
}

/**
 * @brief Parses rest of an assignment  "= expression".
 *
 * Rule 51: AFTER_IDENT -> T_OP_ASSIGN EXPR
 *
 * @param p Parser
 * @param name Name of the variable
 *
 * @return Assignment node (NODE_ASSIGN)
 */
ASTNode *parse_after_ident(Parser *p, char *name) {
    switch (p->token.type) {
    case T_OP_ASSIGN: {
        advance(p);
        ASTNode *after_assign = create_node(NODE_ASSIGN);
        after_assign->data.assign.name = name;
        after_assign->data.assign.value = parse_expression(p, EXPR_STATEMENT);
        return after_assign;
    }
    default:
        throw_fatal_error(2, "After identifier: expected '='");
    }
    return NULL;
}

/**
 * @brief Parses the value after return (can be missing).
 *
 * Rule 52: RETURN_VAL -> EXPR
 * Rule 53: RETURN_VAL -> ε
 *
 * @param p Parser
 *
 * @return Expression node, or NULL if there is no value
 */
ASTNode *parse_return_val(Parser *p) {
    switch (p->token.type) {
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

/**
 * @brief Parses val or var.
 *
 * Rule 54: DEF_VALUE -> T_KW_VAL
 * Rule 55: DEF_VALUE -> T_KW_VAR
 *
 * @param p Parser
 *
 * @return True for var, false for val
 */
static bool parse_def_value(Parser *p) {
    switch (p->token.type) {
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

/**
 * @brief Parses if or while.
 *
 * Rule 56: COMPOUND_STMT -> IF_STMT
 * Rule 57: COMPOUND_STMT -> T_KW_WHILE EXPR T_KW_DO BLOCK
 *
 * @param p Parser
 *
 * @return If node or while node
 */
ASTNode *parse_compound_stmt(Parser *p) {
    switch (p->token.type) {
    case T_KW_IF: {
        return parse_if_stmt(p);
    }
    case T_KW_WHILE: {
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

/**
 * @brief Parses an if statement.
 *
 * Rule 58: IF_STMT -> T_KW_IF EXPR T_KW_THEN BLOCK ELSE_STMT
 *
 * @param p Parser
 *
 * @return If node (NODE_IF)
 */
ASTNode *parse_if_stmt(Parser *p) {
    switch (p->token.type) {
    case T_KW_IF: {
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

/**
 * @brief Parses an else part of if statement.
 *
 * If there is no else, else_block stays NULL.
 *
 * Rule 59: ELSE_STMT -> T_KW_ELSE ELSE_BODY
 * Rule 60: ELSE_STMT -> ε
 *
 * @param p Parser
 * @param if_node If node
 */
static void parse_else_stmt(Parser *p, ASTNode *if_node) {
    switch (p->token.type) {
    case T_KW_ELSE: {
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
    case T_DEDENT: {
        break;
    }
    default:
        throw_fatal_error(2, "After if block: expected 'else', next statement or end of block");
    }
}

/**
 * @brief Parses what comes after else: a block or another if
 *
 * Rule 61: ELSE_BODY -> BLOCK
 * Rule 62: ELSE_BODY -> IF_STMT
 *
 * @param p Parser
 *
 * @return Block node (NODE_BLOCK)
 */
ASTNode *parse_else_body(Parser *p) {
    switch (p->token.type) {
    case T_NEWLINE: {
        return parse_block(p);
    }
    case T_KW_IF: {
        ASTNode *else_block = create_node(NODE_BLOCK);
        ast_array_append(&else_block->data.block.statements, &else_block->data.block.count,
                         &else_block->data.block.capacity, parse_if_stmt(p));
        return else_block;
    }
    default:
        throw_fatal_error(2, "After 'else': expected newline with block or 'if'");
    }
    return NULL;
}