//
// Created by Šimon on 3/11/2026
//
#include "ast.h"
#include <stdio.h>
#include <string.h>

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
static void print_formatted_3(const char *instruction, const char *op1, const char *op2, const char *op3) {
    fprintf(stdout, "%s %s %s %s\n", instruction, op1, op2, op3);
}

/**
 * @brief Formats label for printing
 * 
 * @param fun_name name of the function from AST
 * @param label formatted label ($<fun_name>)
 * @param label_size size of the ouput buffer
 */
static void label_formatter(const char *fun_name, char *label, size_t label_size) {
    snprintf(label, label_size,"$%s", fun_name);
}

/**
 * @brief Formats variable for printing
 * 
 * @param var_name name of the function from AST
 * @param variable formatted variable (LF@<var_name>)
 * @param var_size size of the ouput buffer
 */
static void variable_formatter(const char *var_name, char *variable, size_t var_size) {
    snprintf(variable, var_size,"LF@%s", var_name);
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
    switch (body->type)
            {
            case NODE_BLOCK:
                for (size_t i = 0; i < body->data.block.count; i++) {
                    var_seeker(body->data.block.statements[i]);
                }
                break;
            case NODE_DEF_VAR: {
                char variable[128];
                variable_formatter(body->data.def_var.name, variable, sizeof(variable));
                print_formatted_1("DEFVAR", variable);
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

int generator(ASTNode *ast) {
    fprintf(stdout, ".IFJcode26\n"); // mandatory first line of the program
    bool main_found = false;
    //selects main function and outputs jump to designated function
    for (size_t i = 0; i < ast->data.program.count; i++) {
        ASTNode *cur_fun = ast->data.program.functions[i];
        if (cur_fun->data.function_def.is_main) {
            main_found = true;
            char label[128];
            label_formatter(cur_fun->data.function_def.name, label, sizeof(label));
            print_formatted_1("JUMP", label);
            break;
        }
    }
    // main loop for generating. one cycle for one function
    for (size_t i = 0; i < ast->data.program.count; i++) {
        ASTNode *cur_fun = ast->data.program.functions[i];
        char label[128];
        label_formatter(cur_fun->data.function_def.name, label, sizeof(label));
        print_formatted_1("LABEL", label);
        print_formatted_0("CREATEFRAME");
        print_formatted_0("PUSHFRAME");
        // defines variables and fills then with values, for every parameter
        for (size_t i = 0; i < cur_fun->data.function_def.param_count; i++) {
            ASTNode *param = cur_fun->data.function_def.parameters[i];
            char variable[128];
            variable_formatter(param->data.param.name, variable, sizeof(variable));
            print_formatted_1("DEFVAR", variable);
            print_formatted_1("POPS", variable);
        }
        ASTNode *body = cur_fun->data.function_def.body;
        var_seeker(body);
        print_formatted_0("POPFRAME");
        print_formatted_0("RETURN");
    }
    return 0;
}
