//
// Created by Šimon on 3/11/2026
//
#include "ast.h"
#include "dynamic_string.h"
#include <stdio.h>
#include <string.h>
// Maximum number of characters in a number
#define INT_CHAR_COUNT 21

/**
 * @brief Prints formatted line of code for zero-operand instructions
 *
 * @param instruction printed instruction
 */
static void print_formatted_0(const char *instruction) { fprintf(stdout, "%s\n", instruction); }

/**
 * @brief Prints formatted line of code for one-operand instructions
 *
 * @param instruction printed instruction
 * @param op1 first operand
 */
static void print_formatted_1(const char *instruction, const char *op1) {
    fprintf(stdout, "%s %s\n", instruction, op1);
}

/**
 * @brief Prints formatted line of code for two-operand instructions
 *
 * @param instruction printed instruction
 * @param op1 first operand
 * @param op2 second operand
 */
static void print_formatted_2(const char *instruction, const char *op1, const char *op2) {
    fprintf(stdout, "%s %s %s\n", instruction, op1, op2);
}

/**
 * @brief Prints formatted line of code for three-operand instructions
 *
 * @param instruction printed instruction
 * @param op1 first operand
 * @param op2 second operand
 * @param op3 third operand
 */
static void print_formatted_3(const char *instruction, const char *op1, const char *op2,
                              const char *op3) {
    fprintf(stdout, "%s %s %s %s\n", instruction, op1, op2, op3);
}

/**
 * @brief Formats function label for printing
 *
 * @param label formatted label ($<fun_name>)
 * @param fun_name name of the function from AST
 */
static void fun_label_formatter(DynamicString *label, const char *fun_name) {
    dyn_str_add(label, '$');
    dyn_str_add_string(label, fun_name);
}

/**
 * @brief Formats condition label for printing
 * 
 * @param label formatted label ($<label_count>$<node_type>$<label_type>)
 * @param cond_type type of condition if/while
 * @param label_count dynamic number of labels in a program
 * @param label_type purpose specifying suffix
 */
static void cond_label_formatter(DynamicString *label, NodeType cond_type, size_t label_count, const char *label_type) {
    char label_count_buffer[INT_CHAR_COUNT];
    snprintf(label_count_buffer, sizeof(label_count_buffer),"%zu", label_count);
    dyn_str_add(label, '$');
    dyn_str_add_string(label, label_count_buffer);
    dyn_str_add(label, '$');
    if (cond_type == NODE_IF) {
        dyn_str_add_string(label, "if");
    } else {
        dyn_str_add_string(label, "while");
    }
    dyn_str_add(label, '$');
    dyn_str_add_string(label, label_type);
}

/**
 * @brief Formats variable for printing
 *
 * @param variable formatted variable (LF@<var_name>)
 * @param var_name name of the function from AST
 */
static void variable_formatter(DynamicString *variable, const char *var_name) {
    dyn_str_add_string(variable, "LF@");
    dyn_str_add_string(variable, var_name);
}

/**
 * @brief calculates the label counter within the program
 * 
 * @return current number of labels
 */
static size_t label_counter(void) {
    static size_t label_count = 1;
    return label_count++;
}

/**
 * @brief Seeks every variable and sends it to printer
 *
 * Recursively goes through AST to locate all local variables of the function
 * and send them to printing function
 *
 * @param body body of the function represented by AST
 */
static void var_seeker(ASTNode *body) {
    if (body == NULL) {
        return;
    }
    switch (body->type) {
    case NODE_BLOCK:
        for (size_t i = 0; i < body->data.block.count; i++) {
            var_seeker(body->data.block.statements[i]);
        }
        break;
    case NODE_DEF_VAR: {
        DynamicString *variable = dyn_str_init();
        variable_formatter(variable, body->data.def_var.name);
        print_formatted_1("DEFVAR", variable->string);
        dyn_str_free(variable);
        break;
    }
    case NODE_IF:
        var_seeker(body->data.if_stmt.then_block);
        if (body->data.if_stmt.else_block != NULL) {
            var_seeker(body->data.if_stmt.else_block);
        }
        break;
    case NODE_WHILE:
        var_seeker(body->data.while_loop.body);
        break;
    default:
        break;
    }
}

/**
 * @brief selects appropriate instruction for operation
 *
 * @param operation operation in processed expression
 */
static void operation_formatter(Operator operation) {
    switch (operation) {
    case OP_ADD:
        print_formatted_0("ADDS");
        break;
    case OP_DIV: // type control DIV/IDIV
        print_formatted_0("DIVS");
        break;
    case OP_EQ:
        print_formatted_0("EQS");
        break;
    case OP_GE:
        print_formatted_0("LTS");
        print_formatted_0("NOTS");
        break;
    case OP_SUBB:
        print_formatted_0("SUBS");
        break;
    case OP_GT:
        print_formatted_0("GTS");
        break;
    case OP_LE:
        print_formatted_0("GTS");
        print_formatted_0("NOTS");
        break;
    case OP_LS:
        print_formatted_0("LTS");
        break;
    case OP_MUL:
        print_formatted_0("MULS");
        break;
    case OP_NEQ:
        print_formatted_0("EQS");
        print_formatted_0("NOTS");
        break;
    case OP_UNARY_MINUS: // TODO - type control regarding type of operand vs -1
        print_formatted_1("PUSHS", "int@-1");
        print_formatted_0("MULS");
        break;
    case OP_UNARY_PLUS:
        break;
    default:
        break;
    }
}

/**
 * @brief generates instruction for function body
 *
 * Recursively goes through body AST and based on the node type outputs instructions to printing
 * functions
 *
 * @param body body of the currently processed function
 */
static void instruction_gen(ASTNode *body) {
    if (body == NULL) {
        return;
    }
    switch (body->type) {
    case NODE_BLOCK:
        for (size_t i = 0; i < body->data.block.count; i++) {
            instruction_gen(body->data.block.statements[i]);
        }
        break;
    case NODE_FUNCTION_CALL: {
        for (size_t i = body->data.function_call.arg_count; i > 0; i--) {
            instruction_gen(body->data.function_call.arguments[i - 1]);
        }
        DynamicString *label = dyn_str_init();
        fun_label_formatter(label, body->data.function_call.name);
        print_formatted_1("CALL", label->string);
        dyn_str_free(label);
        break;
    }
    case NODE_BINOP:
        instruction_gen(body->data.bin_op.left);
        instruction_gen(body->data.bin_op.right);
        operation_formatter(body->data.bin_op.operation);
        break;
    case NODE_ASSIGN: {
        instruction_gen(body->data.assign.value);
        DynamicString *variable = dyn_str_init();
        variable_formatter(variable, body->data.assign.name);
        print_formatted_1("POPS", variable->string);
        dyn_str_free(variable);
        break;
    }
    case NODE_RETURN:
        if (body->data.return_stmt.value != NULL) {
            instruction_gen(body->data.return_stmt.value);
        }
        print_formatted_0("POPFRAME");
        print_formatted_0("RETURN");
        break;
    case NODE_IF: {
        instruction_gen(body->data.if_stmt.condition);
        DynamicString *label = dyn_str_init();
        size_t label_count = label_counter();
        cond_label_formatter(label, NODE_IF, label_count, "else");
        print_formatted_1("PUSHS", "bool@false");
        print_formatted_1("JUMPIFEQS", label->string);
        instruction_gen(body->data.if_stmt.then_block);
        DynamicString *label_end = dyn_str_init();
        cond_label_formatter(label_end, NODE_IF,label_count, "end");
        print_formatted_1("JUMP", label_end->string);
        print_formatted_1("LABEL", label->string);
        dyn_str_free(label);
        if (body->data.if_stmt.else_block != NULL) {
            instruction_gen(body->data.if_stmt.else_block);
        }
        print_formatted_1("LABEL", label_end->string);
        dyn_str_free(label_end);
        break;
    }
    case NODE_WHILE: {
        DynamicString *label_start = dyn_str_init();
        size_t label_count = label_counter();
        cond_label_formatter(label_start, NODE_WHILE, label_count, "start");
        print_formatted_1("LABEL", label_start->string);
        instruction_gen(body->data.while_loop.condition);
        print_formatted_1("PUSHS", "bool@false");
        DynamicString *label_end = dyn_str_init();
        cond_label_formatter(label_end, NODE_WHILE, label_count, "end");
        print_formatted_1("JUMPIFEQS", label_end->string);
        instruction_gen(body->data.while_loop.body);
        print_formatted_1("JUMP", label_start->string);
        print_formatted_1("LABEL", label_end->string);
        dyn_str_free(label_start);
        dyn_str_free(label_end);
        break;
    }                                                                                                                                                                                                                                           
    }
}

int generator(ASTNode *ast) {
    fprintf(stdout, ".IFJcode26\n"); // mandatory first line of the program
    bool main_found = false;
    // selects main function and outputs jump to designated function
    for (size_t i = 0; i < ast->data.program.count; i++) {
        ASTNode *cur_fun = ast->data.program.functions[i];
        if (cur_fun->data.function_def.is_main) {
            main_found = true;
            DynamicString *label = dyn_str_init(); 
            fun_label_formatter(label, cur_fun->data.function_def.name);
            print_formatted_1("JUMP", label->string);
            dyn_str_free(label);
            break;
        }
    }
    // main loop for generating. one cycle for one function
    for (size_t i = 0; i < ast->data.program.count; i++) {
        ASTNode *cur_fun = ast->data.program.functions[i];
        DynamicString *label = dyn_str_init();
        fun_label_formatter(label, cur_fun->data.function_def.name);
        print_formatted_1("LABEL", label->string);
        print_formatted_0("CREATEFRAME");
        print_formatted_0("PUSHFRAME");
        //defines variables and fills then with values, for every parameter
        for (size_t x = 0; x < cur_fun->data.function_def.param_count; x++) {
            ASTNode *param = cur_fun->data.function_def.parameters[x];
            DynamicString *variable = dyn_str_init();
            variable_formatter(variable, param->data.param.name);
            print_formatted_1("DEFVAR", variable->string);
            print_formatted_1("POPS", variable->string);
            dyn_str_free(variable);
        }
        ASTNode *body = cur_fun->data.function_def.body;
        var_seeker(body);
        instruction_gen(body);
        print_formatted_0("POPFRAME");
        print_formatted_0("RETURN");
        dyn_str_free(label);
    }
    return 0;
}
