/**
 * @file expression_stack.h
 * @brief Stack interface for expression parsing
 *
 * Defines stack items, markers and groups of items used when reducing
 * expressions into AST nodes.
 *
 * @author Filip Halaš (xhalasf00)
 * @date 27.10.2026
 */

#ifndef IFJ_EXPRESSION_STACK_H
#define IFJ_EXPRESSION_STACK_H

#include "ast.h"
#include "expression_parser.h"
#include "token.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Maximum number of items in one expression handle
 */
#define MAX_EXPR_HANDLE_CAPACITY 3

/**
 * @brief Type of item stored on the expression stack
 */
typedef enum {
    EXPR_TERMINAL,    // Input symbol used in precedence comparisons
    EXPR_NONTERMINAL, // Reduced expression represented by an AST node
    EXPR_MARKER,      // Marks the start of a group of items to reduce
} ExprItemType;

/**
 * @brief Terminal, reduced expression or marker stored on the stack
 *
 * Only fields needed for the given item type should be used.
 */
typedef struct {
    ExprItemType type;     // Determines how the item is used
    PrecTerminal terminal; // Precedence category of a terminal item
    Token token;           // Token associated with a terminal item
    ASTNode *node;         // AST node for a parsed operand or reduced expression
} ExprItem;

/**
 * @brief Group of expression items to be reduced into one AST node
 */
typedef struct {
    ExprItem items[MAX_EXPR_HANDLE_CAPACITY]; // Items in expression order after a successful read
    size_t count;                             // Number of items stored in the handle
} ExprHandle;

/**
 * @brief Linked node containing one expression stack item
 */
typedef struct ExprStackNode {
    ExprItem data;              // Item stored by value
    struct ExprStackNode *next; // Next node towards the bottom of the stack
} ExprStackNode;

/**
 * @brief Stack used by the expression parser
 */
typedef struct {
    ExprStackNode *top; // Top node, or NULL for an empty stack
} ExprStack;

/**
 * @brief Initializes an empty expression stack
 *
 * @param s Stack to initialize; must not be NULL
 */
void init_expr_stack(ExprStack *s);

/**
 * @brief Returns the top item without removing it
 *
 * @param s Initialized stack to read; must not be NULL
 *
 * @return Copy of the top item
 */
ExprItem expr_stack_top(ExprStack *s);

/**
 * @brief Pushes an item onto the expression stack
 *
 * @param s Initialized stack to modify; must not be NULL
 * @param d Item to store
 */
void expr_stack_push(ExprStack *s, ExprItem d);

/**
 * @brief Removes all nodes and leaves the expression stack empty
 *
 * Frees stack nodes, but does not free their token data, AST nodes
 * or the stack structure itself.
 *
 * @param s Initialized stack to clear; must not be NULL
 */
void clear_expr_stack(ExprStack *s);

/**
 * @brief Inserts a marker directly above the topmost terminal
 *
 * Items already above that terminal remain above the new marker.
 *
 * @param s Initialized stack to modify; must not be NULL
 */
void expr_stack_insert_marker(ExprStack *s);

/**
 * @brief Returns the precedence category of the topmost terminal
 *
 * Skips nonterminal items and markers without modifying the stack.
 *
 * @param s Initialized stack to search; must not be NULL
 *
 * @return Precedence category of the first terminal found from the top
 */
PrecTerminal expr_stack_top_terminal(ExprStack *s);

/**
 * @brief Copies the items above the nearest marker into a handle
 *
 * On success, stores items in their expression order and excludes the marker.
 * Does not modify the stack.
 *
 * @param stack Initialized stack to read; must not be NULL
 * @param handle Handle to fill; must not be NULL
 *
 * @return true if a non-empty handle fits within MAX_EXPR_HANDLE_CAPACITY
 *         and a marker is found, otherwise false
 */
bool expr_stack_get_handle(ExprStack *stack, ExprHandle *handle);

/**
 * @brief Replaces the items above the nearest marker with one item
 *
 * Removes those items and the marker, then pushes the replacement.
 *
 * @param stack Initialized stack to modify; must not be NULL
 * @param item Replacement item to push
 *
 * @note Frees removed stack nodes, but not their token data or AST nodes.
 */
void expr_stack_replace_handle(ExprStack *stack, ExprItem item);

#endif // IFJ_EXPRESSION_STACK_H
