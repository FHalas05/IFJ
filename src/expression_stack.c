//
// Created by Filip Halaš on 27/10/2026.
//

#include "expression_stack.h"
#include "expression_parser.h"
#include "library.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void init_expr_stack(ExprStack *s) { s->top = NULL; }

static bool expr_stack_is_empty(ExprStack *s) { return s->top == NULL; }

static ExprStackNode *create_expr_stack_node(ExprItem d, ExprStackNode *next) {
    ExprStackNode *new = (ExprStackNode *)malloc(sizeof(ExprStackNode));
    if (new == NULL) {
        throw_fatal_error(99, "Could not allocate memory!\n");
    }
    new->data = d;
    new->next = next;
    return new;
}

ExprItem expr_stack_top(ExprStack *s) {
    if (s->top == NULL) {
        throw_fatal_error(99, "Stack is empty!\n");
    }
    return s->top->data;
}

void expr_stack_push(ExprStack *s, ExprItem d) { s->top = create_expr_stack_node(d, s->top); }

static void expr_stack_pop(ExprStack *s) {
    if (s->top != NULL) {
        ExprStackNode *elem = s->top;
        s->top = elem->next;
        free(elem);
    }
}

void clear_expr_stack(ExprStack *s) {
    while (!expr_stack_is_empty(s)) {
        expr_stack_pop(s);
    }
}

// helpful function for testing
static const char *terminal_label(const ExprItem *data) {
    if (data->terminal == P_END) {
        return "$";
    }
    if (data->terminal == P_UNARY_MINUS) {
        return "~";
    }

    // Prefer the original operator over its shared precedence category.
    switch (data->token.type) {
    case T_OP_PLUS:
        return "+";
    case T_OP_MINUS:
        return "-";
    case T_OP_MUL:
        return "*";
    case T_OP_DIV:
        return "/";
    case T_OP_LESS:
        return "<";
    case T_OP_LESS_EQ:
        return "<=";
    case T_OP_GR:
        return ">";
    case T_OP_GR_EQ:
        return ">=";
    case T_OP_EQ:
        return "==";
    case T_OP_NOT_EQ:
        return "!=";
    default:
        break;
    }

    // Also support stack items constructed without an original token.
    switch (data->terminal) {
    case P_ADD:
        return "+/-";
    case P_MUL:
        return "*/";
    case P_REL:
        return "REL";
    case P_EQ:
        return "EQ";
    case P_LPAREN:
        return "(";
    case P_RPAREN:
        return ")";
    case P_OPERAND:
        return "i";
    default:
        return "?terminal";
    }
}
// helpful function for testing
void print_stack(const ExprStack *s) {
    if (s == NULL) {
        fputs("STACK: (null)\n", stderr);
        return;
    }

    fputs("STACK top", stderr);

    for (const ExprStackNode *elem = s->top; elem != NULL; elem = elem->next) {
        fputs(" -> ", stderr);

        switch (elem->data.type) {
        case EXPR_TERMINAL:
            fputs(terminal_label(&elem->data), stderr);
            break;
        case EXPR_NONTERMINAL:
            fputs("E", stderr);
            break;
        case EXPR_MARKER:
            fputs("MARK(<)", stderr);
            break;
        default:
            fputs("?item", stderr);
            break;
        }
    }

    fputs(s->top == NULL ? " -> (empty)\n" : " -> bottom\n", stderr);
}

static ExprStackNode *find_top_terminal_node(ExprStack *s) {
    ExprStackNode *current_node = s->top;
    while (current_node != NULL) {
        if (current_node->data.type == EXPR_TERMINAL) {
            return current_node;
        }
        current_node = current_node->next;
    }
    return NULL;
}

PrecTerminal expr_stack_top_terminal(ExprStack *s) {
    ExprStackNode *top_teminal = find_top_terminal_node(s);
    if (top_teminal == NULL) {
        throw_fatal_error(99, "No terminal on expression stack!\n");
    }
    return top_teminal->data.terminal;
}

void expr_stack_insert_marker(ExprStack *s) {
    ExprStackNode *top_terminal = find_top_terminal_node(s);

    if (top_terminal == NULL) {
        throw_fatal_error(99, "No terminal on expression stack!");
    }

    ExprStackNode *current_node = s->top;

    ExprItem marker = {
        .type = EXPR_MARKER,
    };

    if (current_node == top_terminal) {
        s->top = create_expr_stack_node(marker, current_node);
        return;
    }

    while (current_node->next != NULL) {
        if (current_node->next == top_terminal) {
            current_node->next = create_expr_stack_node(marker, current_node->next);
            return;
        }
        current_node = current_node->next;
    }
}

static void reverse_handle(ExprHandle *handle) {
    size_t left = 0;
    size_t right = handle->count - 1;
    while (left < right) {
        ExprItem tempItem = handle->items[left];
        handle->items[left] = handle->items[right];
        handle->items[right] = tempItem;

        left++;
        right--;
    }
}

bool expr_stack_get_handle(ExprStack *stack, ExprHandle *handle) {
    handle->count = 0;

    ExprStackNode *node = stack->top;

    while (node != NULL && node->data.type != EXPR_MARKER) {
        if (handle->count == MAX_EXPR_HANDLE_CAPACITY) {
            return false;
        }
        handle->items[handle->count] = node->data;
        handle->count++;
        node = node->next;
    }

    if (node == NULL || handle->count == 0) {
        return false;
    }

    reverse_handle(handle);

    return true;
}

void expr_stack_replace_handle(ExprStack *stack, ExprItem replacement) {
    while (!expr_stack_is_empty(stack) && expr_stack_top(stack).type != EXPR_MARKER) {
        expr_stack_pop(stack);
    }

    if (expr_stack_is_empty(stack)) {
        throw_fatal_error(99, "Missing expression marker!");
    }

    expr_stack_pop(stack);
    expr_stack_push(stack, replacement);
}
