/**
 * @file expression_stack.c
 * @brief Expression stack implementation
 *
 * Implements a linked stack with markers for finding and replacing
 * groups of items during expression parsing.
 *
 * @author Filip Halaš (xhalasf00)
 * @date 27.10.2026
 */

#include "expression_stack.h"
#include "expression_parser.h"
#include "library.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

void init_expr_stack(ExprStack *s) {
    s->top = NULL;
}

/**
 * @brief Checks whether the expression stack is empty
 *
 * @param s Initialized stack to check; must not be NULL
 *
 * @return true if the stack has no nodes, otherwise false
 */
static bool expr_stack_is_empty(ExprStack *s) {
    return s->top == NULL;
}

/**
 * @brief Creates a stack node containing an item and a link to the next node
 *
 * @param d Item to store
 * @param next Next node towards the bottom of the stack; may be NULL
 *
 * @return Pointer to the newly allocated node
 */
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

void expr_stack_push(ExprStack *s, ExprItem d) {
    s->top = create_expr_stack_node(d, s->top);
}

/**
 * @brief Removes the top node from the expression stack
 *
 * Does nothing if the stack is empty. Frees only the stack node,
 * leaving its token data and AST node unchanged.
 *
 * @param s Initialized stack to modify; must not be NULL
 */
static void expr_stack_pop(ExprStack *s) {
    if (s->top != NULL) {
        ExprStackNode *elem = s->top;
        // Save the next node as the new top before freeing the old one.
        s->top = elem->next;
        free(elem);
    }
}

void clear_expr_stack(ExprStack *s) {
    while (!expr_stack_is_empty(s)) {
        expr_stack_pop(s);
    }
}

/**
 * @brief Finds the first terminal node from the top of the stack
 *
 * Skips nonterminal items and markers without modifying the stack.
 *
 * @param s Initialized stack to search; must not be NULL
 *
 * @return Pointer to the terminal node, or NULL if no terminal is found
 */
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
        // No items are above the terminal, so the marker becomes the new top.
        s->top = create_expr_stack_node(marker, current_node);
        return;
    }

    while (current_node->next != NULL) {
        if (current_node->next == top_terminal) {
            // Place the marker below existing items and directly above the terminal.
            current_node->next = create_expr_stack_node(marker, current_node->next);
            return;
        }
        current_node = current_node->next;
    }
}

/**
 * @brief Reverses the order of items in a non-empty handle
 *
 * @param handle Handle to reverse; must not be NULL and count must be
 *               between 1 and MAX_EXPR_HANDLE_CAPACITY
 */
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

    // Reading from the stack top gives the opposite of the expression order.
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

    // Remove the marker as well before pushing the reduced expression.
    expr_stack_pop(stack);
    expr_stack_push(stack, replacement);
}
