//
// Created by Filip Halaš on 31/10/2026.
//

#include "expression_parser.h"
#include "expression_stack.h"
#include "library.h"
#include "parser.h"
#include "token.h"
#include <stdbool.h>
#include <stdio.h>

#define PREC_TERMINAL_COUNT 9

static const char precedence_table[PREC_TERMINAL_COUNT][PREC_TERMINAL_COUNT] = {
    //                  +    *    <   ==    (    )    ~    i    $
    [P_ADD] =           {'>', '<', '>', '>', '<', '>', '<', '<', '>'},
    [P_MUL] =           {'>', '>', '>', '>', '<', '>', '<', '<', '>'},
    [P_REL] =           {'<', '<', 'x', '>', '<', '>', '<', '<', '>'},
    [P_EQ] =            {'<', '<', '<', 'x', '<', '>', '<', '<', '>'},
    [P_LPAREN] =        {'<', '<', '<', '<', '<', '=', '<', '<', 'x'},
    [P_RPAREN] =        {'>', '>', '>', '>', 'x', '>', 'x', 'x', '>'},
    [P_UNARY_MINUS] =   {'>', '>', '>', '>', '<', '>', '<', '<', '>'},
    [P_OPERAND] =       {'>', '>', '>', '>', 'x', '>', 'x', 'x', '>'},
    [P_END] =           {'<', '<', '<', '<', '<', 'x', '<', '<', 'x'},
};

PrecTerminal get_precedence_terminal(Token token, bool expected_operand) {
    switch (token.type) {
    case T_INT:
    case T_DOUBLE:
    case T_STRING:
    case T_IDENT:
        return P_OPERAND;
    case T_OP_PLUS:
        return P_ADD;
    case T_OP_MINUS:
        return expected_operand ? P_UNARY_MINUS : P_ADD;
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

static bool is_terminal(ExprItem *item, PrecTerminal terminal) {
    return (item->type == EXPR_TERMINAL) && (item->terminal == terminal);
}

static bool handle_matches_reduction_rule(ExprHandle *handle) {
    ExprItem *items = handle->items;

    if (handle->count == 1) {
        // operand
        return is_terminal(&items[0], P_OPERAND);
    }

    if (handle->count == 2) {
        // ~E
        if (is_terminal(&items[0], P_UNARY_MINUS) && items[1].type == EXPR_NONTERMINAL) {
            return true;
        }

        // ()
        if (is_terminal(&items[0], P_LPAREN) && is_terminal(&items[1], P_RPAREN)) {
            return true;
        }
    }

    if (handle->count == 3) {
        // (E)
        if (is_terminal(&items[0], P_LPAREN) && items[1].type == EXPR_NONTERMINAL &&
            is_terminal(&items[2], P_RPAREN)) {
            return true;
        }

        // E operation E
        if (items[0].type == EXPR_NONTERMINAL && items[2].type == EXPR_NONTERMINAL) {
            return is_terminal(&items[1], P_ADD) || is_terminal(&items[1], P_MUL) ||
                   is_terminal(&items[1], P_REL) || is_terminal(&items[1], P_EQ);
        }
    }

    return false;
}

static void parse_function_call(Parser *parser) {
    expect(parser, T_IDENT, "Expected function name!");
    expect(parser, T_PAR_L, "Expected '(' after function name!");

    if (parser->token.type == T_PAR_R) {
        advance(parser);
        return;
    }

    while (true) {
        parse_expression(parser, EXPR_ARGUMENT);
        if (parser->token.type == T_COMMA) {
            advance(parser);
        } else {
            expect(parser, T_PAR_R, "Expected ',' or ')' after argument!");
            return;
        }
    }
}

void parse_expression(Parser *parser, ExprMode mode) {
    ExprStack stack;
    init_expr_stack(&stack);
    bool expected_operand = true;

    ExprItem start_symbol = {
        .type = EXPR_TERMINAL,
        .terminal = P_END,
        .token = {0},
    };
    expr_stack_push(&stack, start_symbol);

    ExprHandle handle;

    size_t paren_depth = 0;

    while (true) {
        PrecTerminal top_t = expr_stack_top_terminal(&stack);
        PrecTerminal input_terminal;

        bool is_argument_end = mode == EXPR_ARGUMENT && paren_depth == 0 &&
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

            if (top->data.type != EXPR_NONTERMINAL || top->next == NULL ||
                !is_terminal(&top->next->data, P_END) || top->next->next != NULL) {
                throw_fatal_error(2, "Incomplete expression!");
            }

            break;
        }

        switch (precedence_table[top_t][input_terminal]) {
        case '<':
            expr_stack_insert_marker(&stack);
        case '=': {
            bool is_call = parser->token.type == T_IDENT && peek_token_type(parser) == T_PAR_L;

            ExprItem input_data = {
                .type = EXPR_TERMINAL,
                .terminal = input_terminal,
                .token = parser->token,
            };

            if (is_call) {
                parse_function_call(parser);
            }

            expr_stack_push(&stack, input_data);

            if (input_terminal == P_LPAREN) {
                paren_depth++;
            } else if (input_terminal == P_RPAREN) {
                if (paren_depth == 0) {
                    throw_fatal_error(2, "Unexpected ')' in expression!");
                }
                paren_depth--;
            }

            expected_operand = input_terminal != P_OPERAND && input_terminal != P_RPAREN;
            print_stack(&stack);

            if (!is_call) {
                advance(parser);
            }

            break;
        }
        case '>':
            if (!expr_stack_get_handle(&stack, &handle) ||
                !handle_matches_reduction_rule(&handle)) {
                throw_fatal_error(2, "Invalid expression reduction!");
            }

            ExprItem replacement = {
                .type = EXPR_NONTERMINAL,
            };
            expr_stack_replace_handle(&stack, replacement);
            print_stack(&stack);
            break;
        default:
            throw_fatal_error(2, "Invalid operator or operand order in expression!");
            break;
        }
    }

    clear_expr_stack(&stack);
}
