/**
 * @file generator.c
 * @brief Generates IFJcode26 instructions based from the program's AST representation
 *
 * Recursively goes through AST to build all function bodies and flow control statements and
 * generates all builtin functions bodies
 *
 * @author Šimon Hejl (xhejlsi00)
 * @date 3.11.2026
 */
#include "ast.h"
#include "dynamic_string.h"
#include "library.h"
#include <stdio.h>
#include <string.h>

// Maximum number of characters in a number
#define INT_CHAR_COUNT 21 // maximum number of characters in int literal
#define INT_CHAR_PREFIX_COUNT                                                                      \
    26 // maximum number of characters in int literal with IFJcode26
       // prefix (int@...)
#define DOUBLE_CHAR_PREFIX_COUNT                                                                   \
    32 // maximum number of characters in double literal with the IFJcode26 prefix (float@...)

// label for conditions and boolean operations
typedef enum { LABEL_IF, LABEL_WHILE, LABEL_AND, LABEL_OR } labels;

/**
 * @brief Prints formatted line of code for zero-operand instructions
 *
 * @param instruction printed instruction
 */
static void print_formatted_0(const char *instruction) {
    fprintf(stdout, "%s\n", instruction);
}

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
 * format used: $<label_count>$<if | while>$<label_type>
 *
 * @param label_to_be_filled formatted label ($<label_count>$<cond_type>$<label_type>)
 * @param cond_type type of condition if/while/and/or
 * @param label_count dynamic number of labels in a program
 * @param label_type purpose specifying suffix (identifies the type of label this is; while ->
 * "start"|"end"; if -> "else"|"end"; and -> "failed"|"end"; or -> "success"|"end") determines which
 * part of the condition we are labeling
 */
static void cond_label_formatter(DynamicString *label_to_be_filled, labels cond_type,
                                 size_t label_count, const char *label_type) {
    char label_count_buffer[INT_CHAR_COUNT];
    snprintf(label_count_buffer, sizeof(label_count_buffer), "%zu", label_count);

    // formats label to the following format
    // $<label_count>$<if | while>$<label_type>

    dyn_str_add(label_to_be_filled, '$');
    dyn_str_add_string(label_to_be_filled, label_count_buffer);
    dyn_str_add(label_to_be_filled, '$');

    if (cond_type == LABEL_IF)
        dyn_str_add_string(label_to_be_filled, "if");
    else if (cond_type == LABEL_WHILE)
        dyn_str_add_string(label_to_be_filled, "while");
    else if (cond_type == LABEL_AND)
        dyn_str_add_string(label_to_be_filled, "and");
    else
        dyn_str_add_string(label_to_be_filled, "or");

    dyn_str_add(label_to_be_filled, '$');
    dyn_str_add_string(label_to_be_filled, label_type);
}

/**
 * @brief Formats variable for printing
 *
 * @param variable formatted variable (LF@<var_name>)
 * @param var_name name of the function from AST
 * @param var_id variable id
 */
static void variable_formatter(DynamicString *variable, const char *var_name, size_t var_id) {
    dyn_str_add_string(variable, "LF@");
    dyn_str_add_string(variable, var_name);
    dyn_str_add_string(variable, "$");
    char var_id_buffer[INT_CHAR_COUNT];
    snprintf(var_id_buffer, sizeof(var_id_buffer), "%zu", var_id);
    dyn_str_add_string(variable, var_id_buffer);
}

/**
 * @brief formats string for printing
 *
 * @param final_string formatted string (string@<string_lit_value)
 * @param string_lit_value string value from AST
 */
static void string_formatter(DynamicString *final_string, const char *string_lit_value) {
    dyn_str_add_string(final_string, "string@");
    for (size_t i = 0; string_lit_value[i] != '\0'; i++) {
        unsigned char character = (unsigned char)string_lit_value[i];
        if (character <= 32 || character == 35 || character == 92) {
            char special_char_buffer[5];
            snprintf(special_char_buffer, sizeof(special_char_buffer), "\\%03d", character);
            dyn_str_add_string(final_string, special_char_buffer);
        } else {
            dyn_str_add(final_string, character);
        }
    }
}

/**
 * @brief keeps track of label count - increments on call
 *
 * @return current number of labels
 */
static size_t get_label_count_and_increment_it(void) {
    static size_t label_count = 1;
    return label_count++;
}

/**
 * @brief generates all variables in a function body
 *
 * Recursively goes through AST to locate all local variables of the function
 * and send them to printing function
 *
 * @param body body of the function represented by AST
 */
static void variable_generator(ASTNode *body) {
    if (body == NULL) {
        return;
    }
    switch (body->type) {
    case NODE_BLOCK:
        for (size_t i = 0; i < body->data.block.count; i++) {
            variable_generator(body->data.block.statements[i]);
        }
        break;
    case NODE_DEF_VAR: {
        DynamicString *variable = dyn_str_init();
        variable_formatter(variable, body->data.def_var.name, body->data.def_var.variable_id);
        print_formatted_1("DEFVAR", variable->string);
        dyn_str_free(variable);
        break;
    }
    case NODE_IF:
        variable_generator(body->data.if_stmt.then_block);
        if (body->data.if_stmt.else_block != NULL) {
            variable_generator(body->data.if_stmt.else_block);
        }
        break;
    case NODE_WHILE:
        variable_generator(body->data.while_loop.body);
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
    case OP_EQ:
        print_formatted_0("EQS");
        break;
    case OP_GR_EQ:
        print_formatted_0("LTS");
        print_formatted_0("NOTS");
        break;
    case OP_SUB:
        print_formatted_0("SUBS");
        break;
    case OP_GR:
        print_formatted_0("GTS");
        break;
    case OP_LESS_EQ:
        print_formatted_0("GTS");
        print_formatted_0("NOTS");
        break;
    case OP_LESS:
        print_formatted_0("LTS");
        break;
    case OP_MUL:
        print_formatted_0("MULS");
        break;
    case OP_NOT_EQ:
        print_formatted_0("EQS");
        print_formatted_0("NOTS");
        break;
    default:
        break;
    }
}

/**
 * @brief generates all instructions in a function body
 *
 * Recursively goes through body AST and based on the node type outputs instructions to printing
 * functions
 *
 * @param body body of the currently processed function
 */
static void instruction_generator(ASTNode *body, bool is_return) {
    if (body == NULL) {
        return;
    }
    switch (body->type) {
    case NODE_BLOCK: {
        size_t count = body->data.block.count;
        if (count == 0)
            break;
        for (size_t i = 0; i < count; i++) {
            bool statement_is_return = is_return && (i == body->data.block.count - 1);
            ASTNode *statement = body->data.block.statements[i];
            instruction_generator(statement, statement_is_return);
            if (statement->type != NODE_DEF_VAR && statement->type != NODE_ASSIGN &&
                statement->type != NODE_IF && statement->type != NODE_WHILE &&
                statement->type != NODE_RETURN && !statement_is_return) {
                print_formatted_1("POPS", "LF@discard");
            }
        }
        break;
    }
    case NODE_FUNCTION_CALL: {
        for (size_t i = 0; i < body->data.function_call.arg_count; i++) {
            instruction_generator(body->data.function_call.arguments[i], false);
        }
        DynamicString *label = dyn_str_init();
        fun_label_formatter(label, body->data.function_call.name);
        print_formatted_1("CALL", label->string);
        dyn_str_free(label);
        break;
    }
    case NODE_DEF_VAR: {
        instruction_generator(body->data.def_var.value, false);
        DynamicString *variable = dyn_str_init();
        variable_formatter(variable, body->data.def_var.name, body->data.def_var.variable_id);
        print_formatted_1("POPS", variable->string);
        dyn_str_free(variable);
        break;
    }
    case NODE_UNOP: {
        Operator op = body->data.un_op.operation;
        ASTNode *operand = body->data.un_op.operand;
        instruction_generator(operand, false);
        if (op == OP_UNARY_MINUS && operand->data_type == TYPE_DOUBLE) {
            print_formatted_1("PUSHS", "float@-0x1p+0");
            print_formatted_0("MULS");
        } else if (op == OP_UNARY_MINUS && operand->data_type == TYPE_INT) {
            print_formatted_1("PUSHS", "int@-1");
            print_formatted_0("MULS");
        } else if (op == OP_NOT) {
            print_formatted_0("NOTS");
        }
        break;
    }
    case NODE_BINOP: {
        Operator op = body->data.bin_op.operation;
        ASTNode *right = body->data.bin_op.right;
        ASTNode *left = body->data.bin_op.left;
        if (op == OP_AND) {
            size_t and_label_count = get_label_count_and_increment_it();
            DynamicString *and_label_failed = dyn_str_init();
            cond_label_formatter(and_label_failed, LABEL_AND, and_label_count, "failed");
            DynamicString *and_label_end = dyn_str_init();
            cond_label_formatter(and_label_end, LABEL_AND, and_label_count, "end");
            instruction_generator(left, false);
            print_formatted_1("PUSHS", "bool@false");
            print_formatted_1("JUMPIFEQS", and_label_failed->string);
            instruction_generator(right, false);
            print_formatted_1("PUSHS", "bool@false");
            print_formatted_1("JUMPIFEQS", and_label_failed->string);
            print_formatted_1("PUSHS", "bool@true");
            print_formatted_1("JUMP", and_label_end->string);
            print_formatted_1("LABEL", and_label_failed->string);
            print_formatted_1("PUSHS", "bool@false");
            print_formatted_1("LABEL", and_label_end->string);
            dyn_str_free(and_label_failed);
            dyn_str_free(and_label_end);
            break;
        } else if (op == OP_OR) {
            size_t or_label_count = get_label_count_and_increment_it();
            DynamicString *or_label_success = dyn_str_init();
            cond_label_formatter(or_label_success, LABEL_OR, or_label_count, "success");
            DynamicString *or_label_end = dyn_str_init();
            cond_label_formatter(or_label_end, LABEL_OR, or_label_count, "end");
            instruction_generator(left, false);
            print_formatted_1("PUSHS", "bool@true");
            print_formatted_1("JUMPIFEQS", or_label_success->string);
            instruction_generator(right, false);
            print_formatted_1("PUSHS", "bool@true");
            print_formatted_1("JUMPIFEQS", or_label_success->string);
            print_formatted_1("PUSHS", "bool@false");
            print_formatted_1("JUMP", or_label_end->string);
            print_formatted_1("LABEL", or_label_success->string);
            print_formatted_1("PUSHS", "bool@true");
            print_formatted_1("LABEL", or_label_end->string);
            dyn_str_free(or_label_success);
            dyn_str_free(or_label_end);
            break;
        }
        instruction_generator(left, false);
        if (left->data_type == TYPE_INT && right->data_type == TYPE_DOUBLE) {
            print_formatted_0("INT2FLOATS");
        }
        instruction_generator(right, false);
        if (left->data_type == TYPE_DOUBLE && right->data_type == TYPE_INT) {
            print_formatted_0("INT2FLOATS");
        }
        if (op == OP_ADD && left->data_type == TYPE_STRING && right->data_type == TYPE_STRING) {
            print_formatted_1("CALL", "$helpConcat");
            break;
        }
        if (op == OP_DIV && (left->data_type == TYPE_DOUBLE || right->data_type == TYPE_DOUBLE)) {
            print_formatted_0("DIVS");
            break;
        }
        if (op == OP_DIV && left->data_type == TYPE_INT && right->data_type == TYPE_INT) {
            print_formatted_0("IDIVS");
            break;
        }
        operation_formatter(op);
        break;
    }
    case NODE_ASSIGN: {
        instruction_generator(body->data.assign.value, false);
        DynamicString *variable = dyn_str_init();
        variable_formatter(variable, body->data.assign.name, body->data.assign.variable_id);
        print_formatted_1("POPS", variable->string);
        dyn_str_free(variable);
        break;
    }
    case NODE_RETURN: {
        if (body->data.return_stmt.value != NULL) {
            instruction_generator(body->data.return_stmt.value, false);
            if (body->data.return_stmt.discard_value)
                print_formatted_1("POPS", "LF@discard");
        }
        if (body->data.return_stmt.value == NULL || body->data.return_stmt.discard_value)
            print_formatted_1("PUSHS", "nil@nil");
        print_formatted_0("POPFRAME");
        print_formatted_0("RETURN");
        break;
    }
    case NODE_IF: {
        instruction_generator(body->data.if_stmt.condition, false);
        DynamicString *label = dyn_str_init();
        size_t label_count = get_label_count_and_increment_it();
        cond_label_formatter(label, LABEL_IF, label_count, "else");
        print_formatted_1("PUSHS", "bool@false");
        print_formatted_1("JUMPIFEQS", label->string);
        instruction_generator(body->data.if_stmt.then_block, is_return);
        DynamicString *label_end = dyn_str_init();
        cond_label_formatter(label_end, LABEL_IF, label_count, "end");
        print_formatted_1("JUMP", label_end->string);
        print_formatted_1("LABEL", label->string);
        dyn_str_free(label);
        if (body->data.if_stmt.else_block != NULL) {
            instruction_generator(body->data.if_stmt.else_block, is_return);
        }
        print_formatted_1("LABEL", label_end->string);
        dyn_str_free(label_end);
        break;
    }
    case NODE_WHILE: {
        DynamicString *label_start = dyn_str_init();
        size_t label_count = get_label_count_and_increment_it();
        cond_label_formatter(label_start, LABEL_WHILE, label_count, "start");
        print_formatted_1("LABEL", label_start->string);
        instruction_generator(body->data.while_loop.condition, false);
        print_formatted_1("PUSHS", "bool@false");
        DynamicString *label_end = dyn_str_init();
        cond_label_formatter(label_end, LABEL_WHILE, label_count, "end");
        print_formatted_1("JUMPIFEQS", label_end->string);
        instruction_generator(body->data.while_loop.body, false);
        print_formatted_1("JUMP", label_start->string);
        print_formatted_1("LABEL", label_end->string);
        dyn_str_free(label_start);
        dyn_str_free(label_end);
        break;
    }
    case NODE_IDENTIFIER: {
        DynamicString *identifier = dyn_str_init();
        variable_formatter(identifier, body->data.identifier.name,
                           body->data.identifier.variable_id);
        print_formatted_1("PUSHS", identifier->string);
        dyn_str_free(identifier);
        break;
    }
    case NODE_INT_LITERAL: {
        char int_buffer[INT_CHAR_PREFIX_COUNT];
        snprintf(int_buffer, sizeof(int_buffer), "int@%ld", body->data.int_lit.value);
        print_formatted_1("PUSHS", int_buffer);
        break;
    }
    case NODE_DOUBLE_LITERAL: {
        char double_buffer[DOUBLE_CHAR_PREFIX_COUNT];
        snprintf(double_buffer, sizeof(double_buffer), "float@%a", body->data.double_lit.value);
        print_formatted_1("PUSHS", double_buffer);
        break;
    }
    case NODE_STRING_LITERAL: {
        char *string_lit_value = body->data.string_lit.value;
        DynamicString *final_string = dyn_str_init();
        string_formatter(final_string, string_lit_value);
        print_formatted_1("PUSHS", final_string->string);
        dyn_str_free(final_string);
        break;
    }
    case NODE_BOOL_LITERAL: {
        if (body->data.bool_lit.value == false) {
            print_formatted_1("PUSHS", "bool@false");
        } else {
            print_formatted_1("PUSHS", "bool@true");
        }
        break;
    }

    case NODE_UNIT_LITERAL: {
        print_formatted_1("PUSHS", "nil@nil");
        break;
    }
    default: {
        throw_fatal_error(404, "generator error - invalid node type");
        // break;
    }
    }

    if (body->widen_to_double) {
        print_formatted_0("INT2FLOATS");
    }
}
/**
 * @brief a function that performs concatenation two top string on the data stack
 */
static void help_concat() {
    print_formatted_1("LABEL", "$helpConcat");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@right");
    print_formatted_1("POPS", "LF@right");
    print_formatted_1("DEFVAR", "LF@left");
    print_formatted_1("POPS", "LF@left");
    print_formatted_1("DEFVAR", "LF@con_string");
    print_formatted_3("CONCAT", "LF@con_string", "LF@left", "LF@right");
    print_formatted_1("PUSHS", "LF@con_string");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

/**
 * @brief outputs body of builtin function print()
 */
static void builtin_print() {
    print_formatted_1("LABEL", "$print");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@var");
    print_formatted_1("POPS", "LF@var");
    print_formatted_1("DEFVAR", "LF@isnil");
    print_formatted_2("TYPE", "LF@isnil", "LF@var");
    print_formatted_3("JUMPIFEQ", "$print_nil", "string@nil", "LF@isnil");
    print_formatted_1("WRITE", "LF@var");
    print_formatted_1("JUMP", "$print_end");
    print_formatted_1("LABEL", "$print_nil");
    print_formatted_1("WRITE", "string@()");
    print_formatted_1("LABEL", "$print_end");
    print_formatted_1("PUSHS", "nil@nil");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

/**
 * @brief outputs body of builtin function readLine()
 */
static void builtin_readline() {
    print_formatted_1("LABEL", "$readLine");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@read_string");
    print_formatted_1("DEFVAR", "LF@isnil");
    print_formatted_2("READ", "LF@read_string", "string");
    print_formatted_2("TYPE", "LF@isnil", "LF@read_string");
    print_formatted_3("JUMPIFEQ", "$read_string_nil", "string@nil", "LF@isnil");
    print_formatted_1("PUSHS", "LF@read_string");
    print_formatted_1("JUMP", "$read_string_end");
    print_formatted_1("LABEL", "$read_string_nil");
    print_formatted_1("PUSHS", "string@");
    print_formatted_1("LABEL", "$read_string_end");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

/**
 * @brief outputs body of builtin function readInt()
 */
static void builtin_readint() {
    print_formatted_1("LABEL", "$readInt");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@read_int");
    print_formatted_1("DEFVAR", "LF@isnil");
    print_formatted_2("READ", "LF@read_int", "int");
    print_formatted_2("TYPE", "LF@isnil", "LF@read_int");
    print_formatted_3("JUMPIFEQ", "$read_int_nil", "string@nil", "LF@isnil");
    print_formatted_1("PUSHS", "LF@read_int");
    print_formatted_1("JUMP", "$read_int_end");
    print_formatted_1("LABEL", "$read_int_nil");
    print_formatted_1("PUSHS", "int@0");
    print_formatted_1("LABEL", "$read_int_end");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

/**
 * @brief outputs body of builtin function readDouble()
 */
static void builtin_readdouble() {
    print_formatted_1("LABEL", "$readDouble");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@read_double");
    print_formatted_1("DEFVAR", "LF@isnil");
    print_formatted_2("READ", "LF@read_double", "float");
    print_formatted_2("TYPE", "LF@isnil", "LF@read_double");
    print_formatted_3("JUMPIFEQ", "$read_double_nil", "string@nil", "LF@isnil");
    print_formatted_1("PUSHS", "LF@read_double");
    print_formatted_1("JUMP", "$read_double_end");
    print_formatted_1("LABEL", "$read_double_nil");
    print_formatted_1("PUSHS", "float@0x0p+0");
    print_formatted_1("LABEL", "$read_double_end");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

static void builtin_readbool() {
    print_formatted_1("LABEL", "$readBoolean");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@read_bool");
    print_formatted_1("DEFVAR", "LF@isnil");
    print_formatted_2("READ", "LF@read_bool", "bool");
    print_formatted_2("TYPE", "LF@isnil", "LF@read_bool");
    print_formatted_3("JUMPIFEQ", "$read_bool_nil", "string@nil", "LF@isnil");
    print_formatted_1("PUSHS", "LF@read_bool");
    print_formatted_1("JUMP", "$read_bool_end");
    print_formatted_1("LABEL", "$read_bool_nil");
    print_formatted_1("PUSHS", "bool@false");
    print_formatted_1("LABEL", "$read_bool_end");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

/**
 * @brief outputs body of builtin function toStr()
 */
static void builtin_tostr() {
    print_formatted_1("LABEL", "$toStr");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@value");
    print_formatted_1("POPS", "LF@value");
    print_formatted_1("DEFVAR", "LF@type_check");
    print_formatted_1("DEFVAR", "LF@fin_string");
    print_formatted_2("TYPE", "LF@type_check", "LF@value");
    print_formatted_3("JUMPIFEQ", "$tostr_int", "string@int", "LF@type_check");
    print_formatted_3("JUMPIFEQ", "$tostr_float", "string@float", "LF@type_check");
    print_formatted_3("JUMPIFEQ", "$tostr_bool", "string@bool", "LF@type_check");
    print_formatted_3("JUMPIFEQ", "$tostr_nil", "string@nil", "LF@type_check");
    print_formatted_2("MOVE", "LF@fin_string", "LF@value");
    print_formatted_1("JUMP", "$tostr_end");
    print_formatted_1("LABEL", "$tostr_int"); // int
    print_formatted_2("INT2STR", "LF@fin_string", "LF@value");
    print_formatted_1("JUMP", "$tostr_end");
    print_formatted_1("LABEL", "$tostr_float"); // float
    print_formatted_2("FLOAT2STR", "LF@fin_string", "LF@value");
    print_formatted_1("JUMP", "$tostr_end");
    print_formatted_1("LABEL", "$tostr_bool"); // bool
    print_formatted_3("JUMPIFEQ", "$tostr_false", "bool@false", "LF@value");
    print_formatted_2("MOVE", "LF@fin_string", "string@true");
    print_formatted_1("JUMP", "$tostr_end");
    print_formatted_1("LABEL", "$tostr_false");
    print_formatted_2("MOVE", "LF@fin_string", "string@false");
    print_formatted_1("JUMP", "$tostr_end");
    print_formatted_1("LABEL", "$tostr_nil"); // nil
    print_formatted_2("MOVE", "LF@fin_string", "string@()");
    print_formatted_1("JUMP", "$tostr_end");
    print_formatted_1("LABEL", "$tostr_end");
    print_formatted_1("PUSHS", "LF@fin_string");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

/**
 * @brief outputs body of builtin function toInt()
 */
static void builtin_toint() {
    print_formatted_1("LABEL", "$toInt");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@double");
    print_formatted_1("POPS", "LF@double");
    print_formatted_1("DEFVAR", "LF@int");
    print_formatted_2("FLOAT2INT", "LF@int", "LF@double");
    print_formatted_1("PUSHS", "LF@int");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

/**
 * @brief outputs body of builtin function strLength()
 */
static void builtin_strlength() {
    print_formatted_1("LABEL", "$strLength");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@mes_string");
    print_formatted_1("POPS", "LF@mes_string");
    print_formatted_1("DEFVAR", "LF@ln_string");
    print_formatted_2("STRLEN", "LF@ln_string", "LF@mes_string");
    print_formatted_1("PUSHS", "LF@ln_string");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

/**
 * @brief outputs body of builtin function subStr()
 */
static void builtin_substr() {
    print_formatted_1("LABEL", "$subStr");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@end");
    print_formatted_1("POPS", "LF@end");
    print_formatted_1("DEFVAR", "LF@start");
    print_formatted_1("POPS", "LF@start");
    print_formatted_1("DEFVAR", "LF@or_string");
    print_formatted_1("POPS", "LF@or_string");
    print_formatted_1("DEFVAR", "LF@check");
    print_formatted_1("PUSHS", "LF@or_string");
    print_formatted_1("CALL", "$strLength");
    print_formatted_1("DEFVAR", "LF@str_length");
    print_formatted_1("POPS", "LF@str_length");
    print_formatted_3("GT", "LF@check", "LF@start", "int@-1");
    print_formatted_3("JUMPIFNEQ", "$substr_err", "bool@true", "LF@check");
    print_formatted_3("GT", "LF@check", "LF@end", "int@-1");
    print_formatted_3("JUMPIFNEQ", "$substr_err", "bool@true", "LF@check");
    print_formatted_3("GT", "LF@check", "LF@start", "LF@end");
    print_formatted_3("JUMPIFEQ", "$substr_err", "bool@true", "LF@check");
    print_formatted_3("LT", "LF@check", "LF@start", "LF@str_length");
    print_formatted_3("JUMPIFEQ", "$substr_err", "bool@false", "LF@check");
    print_formatted_3("GT", "LF@check", "LF@end", "LF@str_length");
    print_formatted_3("JUMPIFEQ", "$substr_err", "bool@true", "LF@check");
    print_formatted_1("DEFVAR", "LF@fin_string");
    print_formatted_1("DEFVAR", "LF@char");
    print_formatted_2("MOVE", "LF@fin_string", "string@");
    print_formatted_1("LABEL", "$substr_concat");
    print_formatted_3("JUMPIFEQ", "$substr_end", "LF@start", "LF@end");
    print_formatted_3("GETCHAR", "LF@char", "LF@or_string", "LF@start");
    print_formatted_3("CONCAT", "LF@fin_string", "LF@fin_string", "LF@char");
    print_formatted_3("ADD", "LF@start", "LF@start", "int@1");
    print_formatted_1("JUMP", "$substr_concat");
    print_formatted_1("LABEL", "$substr_end");
    print_formatted_1("PUSHS", "LF@fin_string");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
    print_formatted_1("LABEL", "$substr_err");
    print_formatted_1("EXIT", "int@40");
}

/**
 * @brief outputs body of builtin function ord()
 */
static void builtin_ord() {
    print_formatted_1("LABEL", "$ord");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@index");
    print_formatted_1("POPS", "LF@index");
    print_formatted_1("DEFVAR", "LF@value");
    print_formatted_1("POPS", "LF@value");
    print_formatted_1("PUSHS", "LF@value");
    print_formatted_1("CALL", "$strLength");
    print_formatted_1("DEFVAR", "LF@str_length");
    print_formatted_1("POPS", "LF@str_length");
    print_formatted_1("DEFVAR", "LF@check");
    print_formatted_3("EQ", "LF@check", "LF@str_length", "int@0");
    print_formatted_3("JUMPIFEQ", "$ord_ln_pass", "bool@false", "LF@check");
    print_formatted_1("PUSHS", "int@-1");
    print_formatted_1("JUMP", "$ord_failed_end");
    print_formatted_1("LABEL", "$ord_ln_pass");
    print_formatted_3("LT", "LF@check", "LF@index", "LF@str_length");
    print_formatted_3("JUMPIFEQ", "$ord_idx_pass", "bool@true", "LF@check");
    print_formatted_1("PUSHS", "int@-1");
    print_formatted_1("JUMP", "$ord_failed_end");
    print_formatted_1("LABEL", "$ord_idx_pass");
    print_formatted_3("LT", "LF@check", "LF@index", "int@0");
    print_formatted_3("JUMPIFEQ", "$ord_null_pass", "bool@false", "LF@check");
    print_formatted_1("PUSHS", "int@-1");
    print_formatted_1("JUMP", "$ord_failed_end");
    print_formatted_1("LABEL", "$ord_null_pass");
    print_formatted_1("DEFVAR", "LF@ord_int");
    print_formatted_3("STRI2INT", "LF@ord_int", "LF@value", "LF@index");
    print_formatted_1("PUSHS", "LF@ord_int");
    print_formatted_1("LABEL", "$ord_failed_end");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}
/**
 * @brief outputs body of builtin function chr()
 */
static void builtin_chr() {
    print_formatted_1("LABEL", "$chr");
    print_formatted_0("CREATEFRAME");
    print_formatted_0("PUSHFRAME");
    print_formatted_1("DEFVAR", "LF@code");
    print_formatted_1("POPS", "LF@code");
    print_formatted_1("DEFVAR", "LF@fin_char");
    print_formatted_2("INT2CHAR", "LF@fin_char", "LF@code");
    print_formatted_1("PUSHS", "LF@fin_char");
    print_formatted_0("POPFRAME");
    print_formatted_0("RETURN");
}

int generator(ASTNode *ast) {
    fprintf(stdout, ".IFJcode26\n"); // mandatory first line of the program

    // selects main function and outputs "CALL <mainfunc>"
    for (size_t i = 0; i < ast->data.program.count; i++) {
        ASTNode *cur_fun = ast->data.program.functions[i];
        if (cur_fun->data.function_def.is_main) {
            DynamicString *label = dyn_str_init();
            fun_label_formatter(label, cur_fun->data.function_def.name);
            print_formatted_1("CALL", label->string);
            dyn_str_free(label);
            break;
        }
    }

    // final program end - terminates the program without errors
    print_formatted_1("EXIT", "int@0");

    // main loop for generating functions - iterates through all functions in AST
    for (size_t i = 0; i < ast->data.program.count; i++) {
        ASTNode *cur_fun = ast->data.program.functions[i];
        DynamicString *label = dyn_str_init();
        fun_label_formatter(label, cur_fun->data.function_def.name);
        print_formatted_1("LABEL", label->string);
        print_formatted_0("CREATEFRAME");
        print_formatted_0("PUSHFRAME");

        print_formatted_1("DEFVAR", "LF@discard");
        // for every function parameter: define a variable, then set its value from the data stack
        for (size_t x = cur_fun->data.function_def.param_count; x > 0; x--) {
            ASTNode *param = cur_fun->data.function_def.parameters[x - 1];
            DynamicString *variable = dyn_str_init();
            variable_formatter(variable, param->data.param.name, param->data.param.variable_id);

            print_formatted_1("DEFVAR", variable->string);
            print_formatted_1("POPS", variable->string);

            dyn_str_free(variable);
        }

        // function body generation
        ASTNode *body = cur_fun->data.function_def.body;

        variable_generator(body);
        bool is_return = (cur_fun->data.function_def.return_type != TYPE_UNIT);
        instruction_generator(body, is_return);

        if (cur_fun->data.function_def.return_type == TYPE_UNIT) {
            print_formatted_1("PUSHS", "nil@nil");
        }
        print_formatted_0("POPFRAME");
        print_formatted_0("RETURN");
        dyn_str_free(label);
    }

    help_concat();
    builtin_print();
    builtin_readline();
    builtin_readint();
    builtin_readdouble();
    builtin_readbool();
    builtin_tostr();
    builtin_toint();
    builtin_strlength();
    builtin_substr();
    builtin_ord();
    builtin_chr();
    return 0;
}
