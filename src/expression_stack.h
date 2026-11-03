//
// Created by Filip Halaš on 27/10/2026.
//

#ifndef IFJ_EXPRESSION_STACK_H
#define IFJ_EXPRESSION_STACK_H

#include <stdbool.h>
#include <stddef.h>
#include "expression_parser.h"
#include "token.h"

#define MAX_EXPR_HANDLE_CAPACITY 3

typedef enum {
    EXPR_TERMINAL,
    EXPR_NONTERMINAL,
    EXPR_MARKER,
} ExprItemType;

typedef struct {
    ExprItemType type;
    PrecTerminal terminal;
    Token token;
} ExprItem;

typedef struct {
    ExprItem items[MAX_EXPR_HANDLE_CAPACITY];
    size_t count;
} ExprHandle;

typedef struct ExprStackNode {
    ExprItem data;
    struct ExprStackNode *next;
} ExprStackNode;

typedef struct {
    ExprStackNode *top;
} ExprStack;

void init_expr_stack(ExprStack *s);
ExprItem expr_stack_top(ExprStack *s);
void expr_stack_push(ExprStack *s, ExprItem d);
void clear_expr_stack(ExprStack *s);
void expr_stack_insert_marker(ExprStack *s);
PrecTerminal expr_stack_top_terminal(ExprStack *s);
bool expr_stack_get_handle(ExprStack *stack, ExprHandle *handle);
void expr_stack_replace_handle(ExprStack *stack, ExprItem item);

void print_stack(const ExprStack *s);

#endif // IFJ_EXPRESSION_STACK_H
