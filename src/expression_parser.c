/**
 * @file expression_parser.c
 * @brief Expression parser implementation
 *
 * Uses a precedence table and expression stack to build AST nodes
 * for operands, operators, parentheses and function calls.
 *
 * @author Filip Halaš (xhalasf00)
 * @date 31.10.2026
 */

#include "expression_parser.h"
#include "ast.h"
#include "expression_stack.h"
#include "library.h"
#include "parser.h"
#include "token.h"
#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief Number of terminal categories in the precedence table
 */
#define PREC_TERMINAL_COUNT (P_END + 1)

/**
 * @brief Actions chosen from the stack terminal and the input terminal
 *
 * Rows represent the topmost stack terminal and columns the input terminal.
 * '<' inserts a marker and pushes the input, '=' only pushes the input,
 * '>' reduces the items above the nearest marker, and a space is invalid.
 */
static const char precedence_table[PREC_TERMINAL_COUNT][PREC_TERMINAL_COUNT] = {
    //                +    *    <   ==   &&   ||    (    )    ~    i    $
    [P_ADD] =       {'>', '<', '>', '>', '>', '>', '<', '>', '<', '<', '>'},
    [P_MUL] =       {'>', '>', '>', '>', '>', '>', '<', '>', '<', '<', '>'},
    [P_REL] =       {'<', '<', ' ', '>', '>', '>', '<', '>', '<', '<', '>'},
    [P_EQ] =        {'<', '<', '<', ' ', '>', '>', '<', '>', '<', '<', '>'},
    [P_AND] =       {'<', '<', '<', '<', '>', '>', '<', '>', '<', '<', '>'},
    [P_OR] =        {'<', '<', '<', '<', '<', '>', '<', '>', '<', '<', '>'},
    [P_LPAREN] =    {'<', '<', '<', '<', '<', '<', '<', '=', '<', '<', ' '},
    [P_RPAREN] =    {'>', '>', '>', '>', '>', '>', ' ', '>', ' ', ' ', '>'},
    [P_UNARY] =     {'>', '>', '>', '>', '>', '>', '<', '>', '<', '<', '>'},
    [P_OPERAND] =   {'>', '>', '>', '>', '>', '>', ' ', '>', ' ', ' ', '>'},
    [P_END] =       {'<', '<', '<', '<', '<', '<', '<', ' ', '<', '<', ' '},
};

/**
 * @brief Assigns a token to its precedence category
 *
 * Distinguishes unary minus from subtraction and maps expression
 * delimiters to P_END.
 *
 * @param token Token to classify
 * @param expected_operand Whether an operand is expected at this position
 *
 * @return Precedence category of the token
 */
static PrecTerminal get_precedence_terminal(Token token, bool expected_operand) {
    switch (token.type) {
    case T_INT:
    case T_DOUBLE:
    case T_STRING:
    case T_IDENT:
    case T_KW_TRUE:
    case T_KW_FALSE:
        return P_OPERAND;
    case T_OP_PLUS:
        return P_ADD;
    case T_OP_MINUS:
        // When an operand is expected, '-' is a unary operator.
        return expected_operand ? P_UNARY : P_ADD;
    case T_OP_NOT:
        return P_UNARY;
    case T_OP_MUL:
    case T_OP_DIV:
        return P_MUL;
    case T_OP_LESS:
    case T_OP_LESS_EQ:
    case T_OP_GR:
    case T_OP_GR_EQ:
        return P_REL;
    case T_OP_EQ:
    case T_OP_NOT_EQ:
        return P_EQ;
    case T_OP_BIN_AND:
        return P_AND;
    case T_OP_BIN_OR:
        return P_OR;
    case T_PAR_L:
        return P_LPAREN;
    case T_PAR_R:
        return P_RPAREN;
    case T_KW_THEN:
    case T_KW_DO:
    case T_NEWLINE:
    case T_SEMICOLON:
    case T_EOF:
        return P_END;
    default:
        throw_fatal_error(2, "Unexpected token in expression!");
        return P_END;
    }
}

/**
 * @brief Converts a binary operator token type to an AST operator
 *
 * @param type Token type of a supported binary operator
 *
 * @return Matching AST operator
 */
static Operator get_binary_operator(TokenType type) {
    switch (type) {
    case T_OP_PLUS:
        return OP_ADD;
    case T_OP_MINUS:
        return OP_SUB;
    case T_OP_MUL:
        return OP_MUL;
    case T_OP_DIV:
        return OP_DIV;
    case T_OP_LESS:
        return OP_LESS;
    case T_OP_LESS_EQ:
        return OP_LESS_EQ;
    case T_OP_GR:
        return OP_GR;
    case T_OP_GR_EQ:
        return OP_GR_EQ;
    case T_OP_EQ:
        return OP_EQ;
    case T_OP_NOT_EQ:
        return OP_NOT_EQ;
    case T_OP_BIN_AND:
        return OP_AND;
    case T_OP_BIN_OR:
        return OP_OR;
    default:
        throw_fatal_error(99, "Invalid binary operator!");
        return OP_ADD;
    }
}

/**
 * @brief Returns an existing operand node or creates one from its token
 *
 * @param item Operand item to reduce; must not be NULL
 *
 * @return Pointer to the AST node representing the operand
 */
static ASTNode *reduce_operand(ExprItem *item) {
    if (item->node != NULL) {
        // A function call already has an AST node from parsing its arguments.
        return item->node;
    }

    ASTNode *node;

    switch (item->token.type) {
    case T_INT:
        node = create_node(NODE_INT_LITERAL);
        node->data_type = TYPE_INT;
        node->data.int_lit.value = strtol(item->token.data, NULL, 10);
        break;
    case T_DOUBLE:
        node = create_node(NODE_DOUBLE_LITERAL);
        node->data_type = TYPE_DOUBLE;
        node->data.double_lit.value = strtod(item->token.data, NULL);
        break;
    case T_STRING:
        node = create_node(NODE_STRING_LITERAL);
        node->data_type = TYPE_STRING;
        node->data.string_lit.value = item->token.data;
        // Keep the string in the AST so token cleanup does not free it.
        item->token.data = NULL;
        break;
    case T_KW_TRUE:
    case T_KW_FALSE:
        node = create_node(NODE_BOOL_LITERAL);
        node->data_type = TYPE_BOOL;
        node->data.bool_lit.value = item->token.type == T_KW_TRUE ? true : false;
        break;
    case T_IDENT:
        node = create_node(NODE_IDENTIFIER);
        node->data.identifier.name = item->token.data;
        // The AST now owns the name, so token cleanup must not free it.
        item->token.data = NULL;
        break;
    default:
        throw_fatal_error(99, "Invalid expresion operand!");
        break;
    }

    token_free_data_if_applicable(item->token);
    item->token.data = NULL;

    return node;
}

/**
 * @brief Checks whether an item is a terminal of the given category
 *
 * @param item Item to check; must not be NULL
 * @param terminal Expected precedence category
 *
 * @return true if both the item type and terminal category match, otherwise false
 */
static bool is_terminal(ExprItem *item, PrecTerminal terminal) {
    return (item->type == EXPR_TERMINAL) && (item->terminal == terminal);
}

/**
 * @brief Reduces a group of expression items into one AST node
 *
 * Handles operands, unary operations, Unit literals, parentheses
 * and binary operations.
 *
 * @param handle Items in expression order; must not be NULL
 *
 * @return Pointer to the AST node representing the reduced expression
 */
static ASTNode *reduce_handle(ExprHandle *handle) {
    ExprItem *items = handle->items;

    if (handle->count == 1) {
        // A single operand becomes an expression node.
        if (is_terminal(&items[0], P_OPERAND)) {
            return reduce_operand(&items[0]);
        }
    }

    if (handle->count == 2) {
        // A unary operator is followed by one reduced expression.
        if (is_terminal(&items[0], P_UNARY) && items[1].type == EXPR_NONTERMINAL) {
            ASTNode *node = create_node(NODE_UNOP);

            if (items[0].token.type == T_OP_MINUS) {
                node->data.un_op.operation = OP_UNARY_MINUS;
            } else {
                node->data.un_op.operation = OP_NOT;
            }

            node->data.un_op.operand = items[1].node;
            return node;
        }

        // Empty parentheses represent a Unit literal.
        if (is_terminal(&items[0], P_LPAREN) && is_terminal(&items[1], P_RPAREN)) {
            ASTNode *node = create_node(NODE_UNIT_LITERAL);
            node->data_type = TYPE_UNIT;

            return node;
        }
    }

    if (handle->count == 3) {
        // Parentheses keep the inner expression without adding an AST node.
        if (is_terminal(&items[0], P_LPAREN) && items[1].type == EXPR_NONTERMINAL &&
            is_terminal(&items[2], P_RPAREN)) {
            return items[1].node;
        }

        // A binary operation joins the left and right expression nodes.
        if (items[0].type == EXPR_NONTERMINAL && items[2].type == EXPR_NONTERMINAL &&
            (is_terminal(&items[1], P_ADD) || is_terminal(&items[1], P_MUL) ||
             is_terminal(&items[1], P_REL) || is_terminal(&items[1], P_EQ) ||
             is_terminal(&items[1], P_AND) || is_terminal(&items[1], P_OR))) {
            ASTNode *node = create_node(NODE_BINOP);
            node->data.bin_op.operation = get_binary_operator(items[1].token.type);
            node->data.bin_op.left = items[0].node;
            node->data.bin_op.right = items[2].node;

            return node;
        }
    }

    throw_fatal_error(2, "Invalid expression reduction!");
    return NULL;
}

/**
 * @brief Parses a function call and its arguments
 *
 * Reads the function name, parentheses and comma-separated arguments.
 * Leaves the parser at the token following the closing parenthesis.
 *
 * @param parser Parser positioned at the function name; must not be NULL
 *
 * @return Pointer to the AST node representing the function call
 */
static ASTNode *parse_function_call(Parser *parser) {
    ASTNode *node = create_node(NODE_FUNCTION_CALL);

    node->data.function_call.name = parser->token.data;
    // Set to NULL so advance() does not free the name now stored in the call node.
    parser->token.data = NULL;

    expect(parser, T_IDENT, "Expected function name!");
    expect(parser, T_PAR_L, "Expected '(' after function name!");

    if (parser->token.type == T_PAR_R) {
        advance(parser);
        return node;
    }

    while (true) {
        ASTNode *argument = parse_expression(parser, EXPR_ARGUMENT);

        ast_array_append(&node->data.function_call.arguments, &node->data.function_call.arg_count,
                         &node->data.function_call.arg_capacity, argument);

        if (parser->token.type == T_COMMA) {
            // Track only a newline occurring after this comma.
            parser->pending_lf = false;
            advance(parser);

            if (parser->token.type == T_PAR_R) {
                // A trailing comma is allowed only if ')' is on a new line.
                if (!parser->pending_lf) {
                    throw_fatal_error(2, "After last comma, ')' must be on a new line!");
                }
                advance(parser);
                return node;
            }
        } else {
            expect(parser, T_PAR_R, "Expected ',' or ')' after argument!");
            return node;
        }
    }
}

ASTNode *parse_expression(Parser *parser, ExprMode mode) {
    ExprStack stack;
    init_expr_stack(&stack);
    bool expected_operand = true;

    // The end symbol stays at the bottom until one complete expression remains.
    ExprItem start_symbol = {
        .type = EXPR_TERMINAL,
        .terminal = P_END,
        .token = {0},
    };
    expr_stack_push(&stack, start_symbol);

    ExprHandle handle;

    size_t expr_paren_depth = 0;

    while (true) {
        PrecTerminal top_t = expr_stack_top_terminal(&stack);
        PrecTerminal input_terminal;

        // A comma or ')' ends an argument only outside its nested parentheses.
        bool is_argument_end = mode == EXPR_ARGUMENT && expr_paren_depth == 0 &&
                               (parser->token.type == T_COMMA || parser->token.type == T_PAR_R);

        if (is_argument_end) {
            input_terminal = P_END;
        } else {
            input_terminal = get_precedence_terminal(parser->token, expected_operand);
            if (mode == EXPR_ARGUMENT && input_terminal == P_END) {
                throw_fatal_error(2, "Expected ',' or ')' after argument!");
            }
        }

        if (top_t == P_END && input_terminal == P_END) {
            ExprStackNode *top = stack.top;

            // A complete expression leaves exactly one AST item above the end symbol.
            if (top->data.type != EXPR_NONTERMINAL || top->next == NULL ||
                !is_terminal(&top->next->data, P_END) || top->next->next != NULL) {
                throw_fatal_error(2, "Incomplete expression!");
            }

            break;
        }

        switch (precedence_table[top_t][input_terminal]) {
        case '<':
            // Mark the next reduction, then also push the input as in the '=' case.
            expr_stack_insert_marker(&stack);
        case '=': {
            bool is_call = parser->token.type == T_IDENT && peek_token_type(parser) == T_PAR_L;

            ExprItem input_data = {
                .type = EXPR_TERMINAL,
                .terminal = input_terminal,
                .token = {.type = parser->token.type},
            };

            if (is_call) {
                // Parse the whole call now and use its AST node as one operand.
                input_data.node = parse_function_call(parser);
            } else if (input_data.terminal == P_OPERAND) {
                input_data.token.data = parser->token.data;
                // Keep operand data for reduction instead of freeing it in advance().
                parser->token.data = NULL;
            }

            expr_stack_push(&stack, input_data);

            if (input_terminal == P_LPAREN) {
                expr_paren_depth++;
            } else if (input_terminal == P_RPAREN) {
                if (expr_paren_depth == 0) {
                    throw_fatal_error(2, "Unexpected ')' in expression!");
                }
                expr_paren_depth--;
            }

            expected_operand = input_terminal != P_OPERAND && input_terminal != P_RPAREN;

            if (!is_call) {
                // Function call parsing has already moved to the next token.
                advance(parser);
            }

            break;
        }
        case '>':
            // Reduce stack items without consuming the current input token.
            if (!expr_stack_get_handle(&stack, &handle)) {
                throw_fatal_error(2, "Invalid expression reduction!");
            }

            ExprItem replacement = {
                .type = EXPR_NONTERMINAL,
                .node = reduce_handle(&handle),
            };
            expr_stack_replace_handle(&stack, replacement);
            break;
        default:
            throw_fatal_error(2, "Invalid operator or operand order in expression!");
            break;
        }
    }

    ASTNode *node = expr_stack_top(&stack).node;

    // Free only stack nodes; the resulting AST is returned to the caller.
    clear_expr_stack(&stack);

    return node;
}
