//
// Created by Šimon on 3/11/2026
//
#include "ast.h"
#include <stdio.h>
#include <string.h>

static void print_formatter_0(const char *instruction) { fprintf(stdout, "%s\n", instruction); }

static void print_formatter_1(const char *instruction, const char *op1) {
    fprintf(stdout, "%s %s\n", instruction, op1);
}

static void print_formatter_2(const char *instruction, const char *op1, const char *op2) {
    fprintf(stdout, "%s %s %s\n", instruction, op1, op2);
}

static void label_formatter(const char *fun_name) { fprintf(stdout, "LABEL $%s\n", fun_name); }

static void jump_formatter(const char *fun_name) { fprintf(stdout, "JUMP $%s\n", fun_name); }

int generator(ASTNode *ast) {
    fprintf(stdout, ".IFJcode26\n");
    bool main_found = false;

    for (size_t i = 0; i < ast->data.program.count; i++) {
        ASTNode *cur_fun = ast->data.program.functions[i];
        if (cur_fun->data.function_def.is_main) {
            main_found = true;
            jump_formatter(cur_fun->data.function_def.name);
            break;
        }
    }
    for (size_t i = 0; i < ast->data.program.count; i++) {
        ASTNode *cur_fun = ast->data.program.functions[i];
        label_formatter(cur_fun->data.function_def.name);
        print_formatter_0("CREATEFRAME");
        print_formatter_0("PUSHFRAME");

        print_formatter_0("POPFRAME");
        print_formatter_0("RETURN");
    }
    return 0;
}
