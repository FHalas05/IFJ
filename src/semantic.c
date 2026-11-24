/**
 * @file semantic.c
 * @brief Semantic analysis of IFJ26
 *
 * The semantic analysis goes through the AST made by the parser
 * and checks that the program makes sense.
 *
 * It works in two passes:
 * 1. collect_functions - saves all functions (and built-in functions)
 *    into a table. Thanks to this, a function can be called before
 *    it is defined in the file.
 * 2. check_function - checks the body of every function:
 *    variables, assignments, conditions, calls, returns and types.
 *
 * Variables are kept in scopes. Every block (function body, if, while)
 * has its own scope. A scope points to its parent scope, so a variable
 * is searched from the inner block to the outer ones.
 *
 * The type of every expression is saved into the AST (data_type),
 * so the code generator knows which instructions to use.
 *
 * Error codes: 3 undefined, 4 redefinition, 5 wrong arguments,
 * 6 wrong types, 7 assignment to val or parameter.
 *
 * @author Jakub Juřík (xjurikj00)
 * @date 17.11.2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "library.h"
#include "semantic.h"
#include "symtable.h"

static void add_builtin(SemContext *ctx, char *name, DataType return_type, size_t param_count,
                        DataType *params);

static void collect_functions(SemContext *ctx, ASTNode *program);

static void check_function(SemContext *ctx, ASTNode *func);

static void check_block(SemContext *ctx, ASTNode *block);

static void check_statement(SemContext *ctx, ASTNode *stmt);

static DataType check_expression(SemContext *ctx, ASTNode *expr);

static DataType check_call(SemContext *ctx, ASTNode *call);

static DataType check_indirect_call(SemContext *ctx, ASTNode *call, TData *var);

bool types_compatible(DataType expected, DataType actual);

static bool is_numeric(DataType t);

static DataType block_result(ASTNode *block);

static bool ends_with_return(ASTNode *block);

static bool all_branches_unit(ASTNode *block);

static bool types_compatible_fun(DataType expected, const FunType *expected_fun, DataType actual, const FunType *actual_fun);

static FunType *function_signature(ASTNode *func);

static void ensure_checked(SemContext *ctx, ASTNode *def);

/**
 * @brief Runs the whole semantic analysis on the AST.
 *
 * @param program Root of the AST (NODE_PROGRAM)
 */
void semantic_analyze(ASTNode *program) {
    SemContext ctx = {0};

    // Pass 1: save all functions
    collect_functions(&ctx, program);

    // Pass 2: check every function that was not checked yet
    // (some can be checked before by check_call)
    for (size_t i = 0; i < program->data.program.count; i++) {
        ASTNode *func = program->data.program.functions[i];
        if (program->data.program.functions[i]->data.function_def.check_state == 0) {
            check_function(&ctx, func);
        }
    }
    destroy_symtable(ctx.functions);
}

/**
 * @brief Adds one built-in function (print, readInt, ...) to the function table.
 *
 * @param ctx Semantic state
 * @param name Name of the function
 * @param return_type Type the function returns
 * @param param_count Number of parameters
 * @param params Types of the parameters (NULL if there are none)
 */
static void add_builtin(SemContext *ctx, char *name, DataType return_type, size_t param_count,
                        DataType *params) {
    // Copy the parameter types to a new array (the table keeps it)
    DataType *types = NULL;
    if (param_count > 0) {
        types = malloc(param_count * sizeof(DataType));
        if (types == NULL) {
            throw_fatal_error(99, "Memory allocation error");
        }
        for (size_t i = 0; i < param_count; i++) {
            types[i] = params[i];
        }
    }
    // Fill the table entry and insert it
    TData d = {0};
    d.kind = SYMBOL_FUNCTION;
    d.data.function.return_type = return_type;
    d.data.function.param_types = types;
    d.data.function.param_count = param_count;
    d.data.function.is_builtin = true;
    d.data.function.definition_node = NULL; // built-in functions have no AST node
    insert(ctx->functions, name, d);
}

/**
 * @brief First pass: saves all functions into the function table.
 *
 * Built-in functions are added first, so a user function with the same
 * name (e.g. print) is reported as a redefinition.
 *
 * Errors:
 * - 4: function defined twice, two parameters with the same name, more than one @main
 * - 5: @main has parameters
 * - 6: @main has a written return type that is not Unit
 * - 3: there is no @main
 *
 * @param ctx Semantic state
 * @param program Root of the AST
 */
static void collect_functions(SemContext *ctx, ASTNode *program) {
    ctx->functions = create_symtable();

    // Built-in functions (Any = any type)
    add_builtin(ctx, "print", TYPE_UNIT, 1, (DataType[]){TYPE_ANY});
    add_builtin(ctx, "readLine", TYPE_STRING, 0, NULL);
    add_builtin(ctx, "readInt", TYPE_INT, 0, NULL);
    add_builtin(ctx, "readDouble", TYPE_DOUBLE, 0, NULL);
    add_builtin(ctx, "readBoolean", TYPE_BOOL, 0, NULL);
    add_builtin(ctx, "toStr", TYPE_STRING, 1, (DataType[]){TYPE_ANY});
    add_builtin(ctx, "toInt", TYPE_INT, 1, (DataType[]){TYPE_DOUBLE});
    add_builtin(ctx, "strLength", TYPE_INT, 1, (DataType[]){TYPE_STRING});
    add_builtin(ctx, "subStr", TYPE_STRING, 3, (DataType[]){TYPE_STRING, TYPE_INT, TYPE_INT});
    add_builtin(ctx, "ord", TYPE_INT, 2, (DataType[]){TYPE_STRING, TYPE_INT});
    add_builtin(ctx, "chr", TYPE_STRING, 1, (DataType[]){TYPE_INT});

    int main_count = 0;

    // Go through all user functions
    for (size_t i = 0; i < program->data.program.count; i++) {
        ASTNode *func = program->data.program.functions[i];
        char *name = func->data.function_def.name;

        // The name is already in the table -> function defined twice
        if (search(ctx->functions, name) != NULL) {
            throw_fatal_error(4, "Function redefinition");
        }

        // Compare every pair of parameters -> two parameters with the same name
        for (size_t j = 0; j < func->data.function_def.param_count; j++) {
            for (size_t k = j + 1; k < func->data.function_def.param_count; k++) {
                char *name_1 = func->data.function_def.parameters[j]->data.param.name;
                char *name_2 = func->data.function_def.parameters[k]->data.param.name;
                if (strcmp(name_1, name_2) == 0) {
                    throw_fatal_error(4, "Duplicate parameter name");
                }
            }
        }

        // Rules for @main
        if (func->data.function_def.is_main) {
            main_count++;
            if (main_count > 1) {
                throw_fatal_error(4, "Only one @main is allowed");
            } else if (func->data.function_def.param_count > 0) {
                throw_fatal_error(5, "@main must have no parameters");
            } else if ((func->data.function_def.has_explicit_return_type == true) &&
                       (func->data.function_def.return_type != TYPE_UNIT)) {
                throw_fatal_error(6, "@main must return Unit");
            }
        }

        // Make an array with the types of the parameters
        DataType *types = NULL;
        if (func->data.function_def.param_count > 0) {
            types = malloc(func->data.function_def.param_count * sizeof(DataType));
            if (types == NULL) {
                throw_fatal_error(99, "Memory allocation error");
            }
            for (size_t j = 0; j < func->data.function_def.param_count; j++) {
                types[j] = func->data.function_def.parameters[j]->data.param.data;
            }
        }

        // Save the function into the table.
        // If the return type is not written, it is TYPE_UNKNOWN for now
        // and it is found later in check_function.
        TData d = {0};
        d.kind = SYMBOL_FUNCTION;
        d.data.function.return_type = func->data.function_def.return_type;
        d.data.function.param_types = types;
        d.data.function.param_count = func->data.function_def.param_count;
        d.data.function.is_builtin = false;
        d.data.function.definition_node = func; // used to check the function later
        insert(ctx->functions, name, d);
    }
    if (main_count == 0) {
        throw_fatal_error(3, "@main not found");
    }
}

/**
 * @brief Checks the body of one function - second pass.
 *
 * - puts the parameters into a new scope (they cannot be changed),
 * - checks all statements of the body,
 * - checks the value at the end of the body (implicit return),
 * - finds the return type if it is not written.
 *
 * A function can be checked in the middle of another function
 * (check_call does it when it needs the return type), so the old state
 * is saved at the start and put back at the end.
 *
 * check_state: 0 = not checked, 1 = being checked now, 2 = done.
 *
 * @param ctx Semantic state
 * @param func Function node
 */
static void check_function(SemContext *ctx, ASTNode *func) {
    // Save the state of the function that was checked before
    Scope *saved_scope = ctx->current;
    ASTNode *saved_func = ctx->current_function;

    func->data.function_def.check_state = 1;     // being checked now
    func->data.function_def.caller = saved_func; // who started this check (for recursion)
    ctx->current_function = func;

    // New scope without a parent: a function cannot see variables of other functions
    ctx->current = create_scope(NULL);

    // Parameters are variables that cannot be changed.
    // They are in the same scope as the body, so "val x" in the body
    // with a parameter x is a redefinition (4).
    for (size_t i = 0; i < func->data.function_def.param_count; i++) {
        ASTNode *param = func->data.function_def.parameters[i];
        TData d = {0};
        d.kind = SYMBOL_VARIABLE;
        d.data.variable.type = param->data.param.data;
        d.data.variable.fun_type = param->data.param.fun;
        d.data.variable.is_mutable = false;
        d.data.variable.variable_id = ctx->next_var_id++;
        insert(ctx->current->symbols, param->data.param.name, d);

        // Store the new ID in both the symbol table and the AST
        param->data.param.variable_id = d.data.variable.variable_id;
    }

    // Check the statements of the body (not with check_block,
    // because the body uses the scope with the parameters)
    ASTNode *body = func->data.function_def.body;
    for (size_t i = 0; i < body->data.block.count; i++) {
        check_statement(ctx, body->data.block.statements[i]);
    }

    // Last statement of the body = value of the function (if there is no return)
    ASTNode *last = NULL;
    if (body->data.block.count > 0) {
        last = body->data.block.statements[body->data.block.count - 1];
    }

    // Body ends with an expression, val/var, assignment or while.
    // (return was already checked in check_statement, if is checked below)
    if ((last != NULL) && (last->type != NODE_RETURN) && (last->type != NODE_IF)) {
        DataType type = block_result(body);
        DataType ret = func->data.function_def.return_type;
        if (func->data.function_def.has_explicit_return_type) {
            // Written type: the last value must fit it (Unit function throws it away)
            if ((ret != TYPE_UNIT) && (!types_compatible_fun(ret, func->data.function_def.return_fun, type, last->fun_type))) {
                throw_fatal_error(6, "Implicit return value type mismatch");
            }
        } else {
            // Type not written: the function gets the type of the last value.
            // Save it into the AST and into the function table.
            func->data.function_def.return_type = type;
            search(ctx->functions, func->data.function_def.name)->data.function.return_type = type;
            // Function type: the function needs its own copy
            // (last->fun_type belongs to the last expression, both are freed by free_ast)
            if (type == TYPE_FUNCTION){
                func->data.function_def.return_fun = fun_type_copy(last->fun_type);
            }
        }
    }

    // Body ends with if. It is OK only if:
    // - the return type is written and it is Unit, or
    // - all branches end with return, or
    // - the return type is not written and all branches give Unit (type is Unit)
    DataType ret = func->data.function_def.return_type;
    if ((last != NULL) && (last->type == NODE_IF)) {
        if ((func->data.function_def.has_explicit_return_type) && (ret == TYPE_UNIT)) {
            // OK
        } else if (ends_with_return(func->data.function_def.body)) {
            // OK
        } else if ((!func->data.function_def.has_explicit_return_type) &&
                   (all_branches_unit(func->data.function_def.body))) {
            func->data.function_def.return_type = TYPE_UNIT;
            search(ctx->functions, func->data.function_def.name)->data.function.return_type =
                TYPE_UNIT;
        } else {
            throw_fatal_error(6, "If at end of function body must end with return in all branches");
        }
    }

    // @main without a written type: the found type must be Unit
    if ((func->data.function_def.is_main) && (func->data.function_def.return_type != TYPE_UNIT)) {
        throw_fatal_error(6, "@main must return Unit");
    }

    // Close the scope and put back the saved state
    destroy_scope(ctx->current);
    func->data.function_def.check_state = 2; // done
    ctx->current = saved_scope;
    ctx->current_function = saved_func;
}

/**
 * @brief Checks a block (body of if, else or while).
 *
 * The block gets a new scope, so variables defined inside
 * are not visible after the block ends.
 *
 * @param ctx Semantic state
 * @param block Block node (can be NULL, e.g. if without else)
 */
static void check_block(SemContext *ctx, ASTNode *block) {
    // Missing else block -> nothing to check
    if (block == NULL) {
        return;
    }
    // Open a new scope (its parent is the current scope)
    ctx->current = create_scope(ctx->current);

    // Check all statements in the block
    for (size_t i = 0; i < block->data.block.count; i++) {
        ASTNode *stmt = block->data.block.statements[i];
        check_statement(ctx, stmt);
    }

    // Close the scope and go back to the parent scope
    Scope *parent = ctx->current->parent;
    destroy_scope(ctx->current);
    ctx->current = parent;
}

/**
 * @brief Checks one statement.
 *
 * - val/var: type of the value, redefinition in the same block (4)
 * - assignment: variable exists (3), is var (7), type fits (6)
 * - if/while: condition is Boolean (6), then checks the blocks
 * - function call: see check_call
 * - return: return type rules (6)
 * - other statements are expressions
 *
 * @param ctx Semantic state
 * @param stmt Statement node
 */
static void check_statement(SemContext *ctx, ASTNode *stmt) {
    if (stmt == NULL) {
        return;
    }
    switch (stmt->type) {
    // val x: Type = value / var x = value
    case NODE_DEF_VAR: {
        // Check the value first. The new variable does not exist yet,
        // so "val x = x + 1" reports x as undefined (3).
        DataType value_type = check_expression(ctx, stmt->data.def_var.value);
        FunType *value_fun = stmt->data.def_var.value->fun_type;

        DataType var_type = stmt->data.def_var.type;
        FunType *var_fun = stmt->data.def_var.type_fun;

        if (var_type == TYPE_UNKNOWN) {
            // Type is not written -> the variable gets the type of the value
            if (value_type == TYPE_UNKNOWN) {
                throw_fatal_error(6, "Cannot infer variable type");
            }
            if ((stmt->data.def_var.value->type == NODE_IDENTIFIER) && (stmt->data.def_var.value->data.identifier.is_function_ref)){
                throw_fatal_error(6, "Function used as a value needs a written variable type");
            }
            var_type = value_type;
            var_fun = value_fun;
        } else if (!types_compatible_fun(var_type, var_fun, value_type, value_fun)) {
            // Type is written -> the value must fit it
            throw_fatal_error(6, "Type mismatch in variable initialization");
        }

        // Only the current block is searched: the same name in an outer
        // block is allowed (shadowing), in the same block it is not
        char *name = stmt->data.def_var.name;
        if (search(ctx->current->symbols, name) != NULL) {
            throw_fatal_error(4, "Variable redefinition");
        }

        // Save the new variable into the current scope
        TData d = {0};
        d.kind = SYMBOL_VARIABLE;
        d.data.variable.type = var_type;
        d.data.variable.fun_type = var_fun;
        d.data.variable.is_mutable = stmt->data.def_var.var; // true for var, false for val
        d.data.variable.variable_id = ctx->next_var_id++;    // unique number of the variable

        insert(ctx->current->symbols, name, d);

        // Store the new ID in both the symbol table and the AST
        stmt->data.def_var.variable_id = d.data.variable.variable_id;
        break;
    }

    // x = value
    case NODE_ASSIGN: {
        char *name = stmt->data.assign.name;
        // Search in all scopes, from the inner one to the outer ones
        TData *var = search_scope(ctx->current, name);
        if (var == NULL) {
            throw_fatal_error(3, "Undefined variable");
        } else if (!var->data.variable.is_mutable) {
            // val and parameters cannot be changed
            throw_fatal_error(7, "Assignment to val / parameter");
        }

        // Use the ID of the variable found in the current scope
        stmt->data.assign.variable_id = var->data.variable.variable_id;

        DataType type = check_expression(ctx, stmt->data.assign.value);
        if ((!types_compatible_fun(var->data.variable.type, var->data.variable.fun_type , type, stmt->data.assign.value->fun_type))) {
            throw_fatal_error(6, "Wrong type in assign");
        }
        break;
    }

    // if condition then block else block
    case NODE_IF: {
        DataType cond = check_expression(ctx, stmt->data.if_stmt.condition);
        if (cond != TYPE_BOOL) {
            throw_fatal_error(6, "Condition must be Boolean");
        }
        // Every branch has its own scope
        check_block(ctx, stmt->data.if_stmt.then_block);
        check_block(ctx, stmt->data.if_stmt.else_block); // NULL if there is no else
        break;
    }

    // while condition do block
    case NODE_WHILE: {
        DataType cond = check_expression(ctx, stmt->data.while_loop.condition);
        if (cond != TYPE_BOOL) {
            throw_fatal_error(6, "Condition must be Boolean");
        }
        check_block(ctx, stmt->data.while_loop.body);
        break;
    }

    // f(args) as a statement; the type is saved for block_result
    case NODE_FUNCTION_CALL: {
        stmt->data_type = check_call(ctx, stmt);
        break;
    }

    // return / return value
    case NODE_RETURN: {
        // A lambda gives the value of its last expression, return is not allowed
        if(ctx->current_function->data.function_def.is_lambda){
            throw_fatal_error(10, "Return is not allowed in a lambda");
        }
        // A function with return must have a written return type
        else if (ctx->current_function->data.function_def.has_explicit_return_type == false) {
            throw_fatal_error(6, "Wrong return type");
        }
        // "return" without a value is allowed only in a Unit function
        else if ((stmt->data.return_stmt.value == NULL) &&
                 (ctx->current_function->data.function_def.return_type != TYPE_UNIT)) {
            throw_fatal_error(6, "Wrong return type");
        }
        // "return value": the value must fit the return type.
        // In a Unit function the value is thrown away, so any type is OK
        // (but the expression is still checked).
        else if (stmt->data.return_stmt.value != NULL) {
            DataType type = check_expression(ctx, stmt->data.return_stmt.value);
            if ((ctx->current_function->data.function_def.return_type != TYPE_UNIT) &&
                (!types_compatible_fun(ctx->current_function->data.function_def.return_type, ctx->current_function->data.function_def.return_fun, type, stmt->data.return_stmt.value->fun_type))) {
                throw_fatal_error(6, "Wrong type in return");
            }
        }
        break;
    }

    // Any other statement is an expression (e.g. "x + 1")
    default:
        check_expression(ctx, stmt);
        break;
    }
}

/**
 * @brief Finds the type of an expression and checks it.
 *
 * Goes through the whole expression (recursively) and saves the type
 * of every part into data_type in the AST.
 *
 * Rules:
 * - +       : two numbers (Int + Int = Int, otherwise Double) or two Strings
 * - - * /   : two numbers
 * - < <= > >=: two numbers or two Strings, result is Boolean
 * - == !=   : two numbers or two values of the same type, result is Boolean
 * - && ||   : two Booleans
 * - !       : Boolean, unary minus: number
 *
 * Errors: 3 undefined variable or function, 5 wrong arguments, 6 wrong types.
 *
 * @param ctx Semantic state
 * @param expr Expression node (NULL means no value, type Unit)
 * @return Type of the expression
 */
static DataType check_expression(SemContext *ctx, ASTNode *expr) {
    if (expr == NULL) {
        return TYPE_UNIT;
    }
    DataType result = TYPE_UNKNOWN;
    switch (expr->type) {
    // Literals: the type is clear
    case NODE_INT_LITERAL:
        result = TYPE_INT;
        break;
    case NODE_DOUBLE_LITERAL:
        result = TYPE_DOUBLE;
        break;
    case NODE_STRING_LITERAL:
        result = TYPE_STRING;
        break;
    case NODE_UNIT_LITERAL:
        result = TYPE_UNIT;
        break;

    // Variable, or a function name used as a value
    case NODE_IDENTIFIER: {
        char *name = expr->data.identifier.name;
        TData *var = search_scope(ctx->current, name);
        if (var != NULL) {
            expr->data.identifier.variable_id = var->data.variable.variable_id;
            result = var->data.variable.type;
            if (result == TYPE_FUNCTION){
                expr->fun_type = fun_type_copy (var->data.variable.fun_type);
            }
            break;
        }

        // Not a variable: it can be a user function used as a value (val f: (Int) => Int = foo)
        TData *func = search(ctx->functions, name);
        if ((func == NULL) || (func->data.function.is_builtin)){
            throw_fatal_error(3, "Undefined variable");
        }

        ASTNode *def = func->data.function.definition_node;
        ensure_checked(ctx, def);

        // Mark it for the generator (it pushes the function label) and for val/var (needs a written type)
        expr->data.identifier.is_function_ref = true;
        expr->fun_type = function_signature(def);
        result = TYPE_FUNCTION;
        break;
    }

    // Binary operator: check both sides first, then the operator rules
    case NODE_BINOP: {
        DataType a = check_expression(ctx, expr->data.bin_op.left);
        DataType b = check_expression(ctx, expr->data.bin_op.right);
        switch (expr->data.bin_op.operation) {
        // + : numbers or Strings (String + String = join)
        case OP_ADD:
            if ((is_numeric(a)) && (is_numeric(b))) {
                if ((a == TYPE_INT) && (b == TYPE_INT)) {
                    result = TYPE_INT;
                } else {
                    result = TYPE_DOUBLE; // Int is converted to Double
                }
            } else if ((a == TYPE_STRING) && (b == TYPE_STRING)) {
                result = TYPE_STRING;
            } else {
                throw_fatal_error(6, "Operator '+' requires two numbers or two strings");
            }
            break;

        // - * / : only numbers
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
            if ((is_numeric(a)) && (is_numeric(b))) {
                if ((a == TYPE_INT) && (b == TYPE_INT)) {
                    result = TYPE_INT; // Int / Int is whole number division
                } else {
                    result = TYPE_DOUBLE;
                }
            } else {
                throw_fatal_error(6, "Arithmetic operator requires numeric operands");
            }
            break;

        // < <= > >= : numbers or Strings
        case OP_GR:
        case OP_GR_EQ:
        case OP_LESS:
        case OP_LESS_EQ:
            if (((is_numeric(a)) && (is_numeric(b))) ||
                ((a == TYPE_STRING) && (b == TYPE_STRING))) {
                result = TYPE_BOOL;
            } else {
                throw_fatal_error(6, "Relational operator requires two numbers or two strings");
            }
            break;

        // == != : two numbers or the same type
        case OP_EQ:
        case OP_NOT_EQ:
            if ((is_numeric(a)) && (is_numeric(b))) {
                result = TYPE_BOOL;
            } else if (a == b) {
                result = TYPE_BOOL;
            } else {
                throw_fatal_error(6, "Equality operator requires compatible operands");
            }
            break;

        // && || : only Booleans (BOOLTHEN)
        case OP_AND:
        case OP_OR: {
            if ((a == TYPE_BOOL) && (b == TYPE_BOOL)) {
                result = TYPE_BOOL;
            } else {
                throw_fatal_error(6, "Logical operator requires Boolean operands");
            }
            break;
        }
        default:
            break;
        }
        break;
    }

    // Unary operator: ! needs a Boolean, minus needs a number
    case NODE_UNOP: {
        DataType t = check_expression(ctx, expr->data.un_op.operand);
        if (expr->data.un_op.operation == OP_NOT) {
            if (t == TYPE_BOOL) {
                result = TYPE_BOOL;
            } else {
                throw_fatal_error(6, "Operator '!' requires a Boolean operand");
            }
        } else {
            if (is_numeric(t)) {
                result = t; // minus does not change the type
            } else {
                throw_fatal_error(6, "Unary minus requires a numeric operand");
            }
        }
        break;
    }

    // Function call inside an expression (FUNEXP)
    case NODE_FUNCTION_CALL:
        result = check_call(ctx, expr);
        break;

    // true / false (BOOLTHEN)
    case NODE_BOOL_LITERAL:
        result = TYPE_BOOL;
        break;
    // Lambda: it is a normal function "lambda$N", the value has its function type
    case NODE_LAMBDA:{
        ASTNode *def = expr->data.lambda.function;
        ensure_checked(ctx, def);
        expr->fun_type = function_signature(def);
        result = TYPE_FUNCTION;
        break; 
    }
    default:
        break;
    }

    // Save the type into the AST for the code generator
    expr->data_type = result;
    return result;
}

/**
 * @brief Checks a function call and returns the type the function returns.
 *
 * - the function must exist (3),
 * - the number of arguments must be right (5),
 * - every argument must fit its parameter type (5).
 *
 * If the called function was not checked yet, it is checked now,
 * so its return type is known (also for functions defined later in the file).
 * If the called function is being checked right now, it is recursion:
 * all functions in the circle must have a written return type (6).
 *
 * @param ctx Semantic state
 * @param call Call node (NODE_FUNCTION_CALL)
 * @return Return type of the called function
 */
static DataType check_call(SemContext *ctx, ASTNode *call) {
    TData *var = search_scope (ctx->current, call->data.function_call.name);
    if ((var != NULL) && (var->data.variable.type == TYPE_FUNCTION)){
        return check_indirect_call(ctx, call, var);
    }
    TData *func = search(ctx->functions, call->data.function_call.name);
    if (func == NULL) {
        throw_fatal_error(3, "Undefined function");
    } else if (func->data.function.param_count != call->data.function_call.arg_count) {
        throw_fatal_error(5, "Wrong count of parameters");
    }

    bool is_builtin = func->data.function.is_builtin;
    ASTNode *def = func->data.function.definition_node;

    // Check every argument and compare it with the parameter type
    for (size_t i = 0; i < call->data.function_call.arg_count; i++) {
         ASTNode *arg = call->data.function_call.arguments[i];
        DataType type = check_expression(ctx, arg);

        DataType expected;
        const FunType *expected_fun;
        if (is_builtin) {
            // Built-in functions have no function parameters.
            // A function value is not accepted even for Any (print, toStr).
            expected = func->data.function.param_types[i];
            expected_fun = NULL;
        } else {
            // User function: the parameter in the AST knows its function type
            ASTNode *param = def->data.function_def.parameters[i];
            expected = param->data.param.data;
            expected_fun = param->data.param.fun;
        }
        if (!types_compatible_fun(expected, expected_fun, type, arg->fun_type)) {
            throw_fatal_error(5, "Wrong argument type");
        }
    }

    // Tell the code generator if this is a built-in function
    call->data.function_call.is_builtin = is_builtin;

    if (!is_builtin) {
        // The return type must be known (checks the function now, or recursion rules)
        ensure_checked(ctx, def);
        // The function returns a function: the call needs its own copy of the type
        if (def->data.function_def.return_type == TYPE_FUNCTION) {
            call->fun_type = fun_type_copy(def->data.function_def.return_fun);
        }
    }
    return func->data.function.return_type;
}

/**
 * @brief Checks a call of a variable of function type, e.g. f(5).
 *
 * The number and types of the arguments are taken from the function type
 * of the variable (5 if they do not fit). The call is marked as indirect,
 * so the generator jumps to the label stored in the variable.
 *
 * @param ctx Semantic state
 * @param call Call node (NODE_FUNCTION_CALL)
 * @param var Variable of function type with the same name as the call
 * @return Return type of the function type
 */
static DataType check_indirect_call(SemContext *ctx, ASTNode *call, TData *var){
    const FunType *fun = var->data.variable.fun_type;
    call->data.function_call.is_indirect = true;
    call->data.function_call.variable_id = var->data.variable.variable_id;

    if ((fun == NULL) || (fun->param_count != call->data.function_call.arg_count)){
        throw_fatal_error(5, "Wrong count of parameters");
    }

    for (size_t i = 0; i < call->data.function_call.arg_count; i++){
        ASTNode *arg = call->data.function_call.arguments[i];
        DataType type = check_expression(ctx, arg);
        if (!types_compatible_fun(fun->params[i].kind, fun->params[i].fun, type, arg->fun_type)){
            throw_fatal_error(5, "Wrong argument type");
        }
    }

    if (fun->ret.kind == TYPE_FUNCTION){
        call->fun_type = fun_type_copy(fun->ret.fun);
    }
    return fun->ret.kind;
}

/**
 * @brief Checks if a value of type actual can be used where expected is needed.
 *
 * True if the types are the same, if expected is Any (print, toStr)
 * or if an Int is used where a Double is needed (the only automatic conversion).
 *
 * @param expected Type that is needed
 * @param actual Type of the value
 * @return true if the value can be used
 */
bool types_compatible(DataType expected, DataType actual) {
    if (expected == actual) {
        return true;
    } else if (expected == TYPE_ANY) {
        return true;
    } else if ((expected == TYPE_DOUBLE) && (actual == TYPE_INT)) {
        return true;
    }
    return false;
}

/**
 * @brief Checks if a type is a number (Int or Double).
 *
 * @param t Type
 * @return true for Int or Double
 */
static bool is_numeric(DataType t) {
    if ((t == TYPE_INT) || (t == TYPE_DOUBLE)) {
        return true;
    }
    return false;
}

/**
 * @brief Returns the value type of a block (type of its last statement).
 *
 * A block that ends with val/var, assignment or while gives Unit.
 * Otherwise it gives the type of the last expression or call.
 *
 * @param block Block node
 * @return Type of the block value
 */
static DataType block_result(ASTNode *block) {
    if (block->data.block.count == 0) {
        return TYPE_UNIT;
    }
    ASTNode *last = block->data.block.statements[block->data.block.count - 1];
    if ((last->type == NODE_DEF_VAR) || (last->type == NODE_ASSIGN) || (last->type == NODE_WHILE)) {
        return TYPE_UNIT;
    } else {
        return last->data_type; // set before by check_expression / check_call
    }
}

/**
 * @brief Checks if all paths of a block end with return.
 *
 * If the block ends with if, both branches are checked (also nested ifs).
 * A missing else does not end with return.
 *
 * @param block Block node (can be NULL)
 * @return true if every branch ends with return
 */
static bool ends_with_return(ASTNode *block) {
    if ((block == NULL) || (block->data.block.count == 0)) {
        return false;
    }
    ASTNode *last = block->data.block.statements[block->data.block.count - 1];
    if (last->type == NODE_RETURN) {
        return true;
    } else if (last->type == NODE_IF) {
        return ends_with_return(last->data.if_stmt.then_block) &&
               ends_with_return(last->data.if_stmt.else_block);
    }
    return false;
}

/**
 * @brief Checks if all branches of a block give Unit.
 *
 * If the block ends with if, both branches are checked (also nested ifs).
 * A missing else counts as () (Unit).
 *
 * @param block Block node (can be NULL)
 * @return true if every branch gives Unit
 */
static bool all_branches_unit(ASTNode *block) {
    if ((block == NULL) || (block->data.block.count == 0)) {
        return true;
    }
    ASTNode *last = block->data.block.statements[block->data.block.count - 1];
    if (last->type == NODE_IF) {
        return all_branches_unit(last->data.if_stmt.then_block) &&
               all_branches_unit(last->data.if_stmt.else_block);
    }
    return block_result(block) == TYPE_UNIT;
}

/**
 * @brief Checks if a value can be used where a type is needed (also function types).
 *
 * If at least one of the types is a function type, both must be function types
 * with the same parameters and return type (no conversion between function types).
 * Otherwise the normal rules of types_compatible are used (e.g. Int -> Double).
 *
 * @param expected Type that is needed
 * @param expected_fun Function type that is needed (NULL if expected is not TYPE_FUNCTION)
 * @param actual Type of the value
 * @param actual_fun Function type of the value (NULL if actual is not TYPE_FUNCTION)
 * @return true if the value can be used
 */
static bool types_compatible_fun(DataType expected, const FunType *expected_fun, DataType actual, const FunType *actual_fun){
    if ((expected == TYPE_FUNCTION) || (actual == TYPE_FUNCTION)){
        return (expected == actual) && fun_type_equal(expected_fun, actual_fun);
    }
    return types_compatible(expected, actual);
}

/**
 * @brief Makes the function type (signature) of a function definition.
 *
 * Used when a function is used as a value: a lambda, or a normal function
 * given by its name (val f: (Int) => Int = foo).
 * The return type must be known, so the function must be checked before.
 *
 * @param func Function definition (NODE_FUNCTION_DEF)
 * @return New function type (the caller owns it and must free it)
 */
static FunType *function_signature(ASTNode *func){
    FunType *sig = create_fun_type(func->data.function_def.param_count);
    for (size_t i = 0; i < func->data.function_def.param_count; i++){
        sig->params[i].kind = func->data.function_def.parameters[i]->data.param.data;
        sig->params[i].fun = fun_type_copy(func->data.function_def.parameters[i]->data.param.fun); 
    }
    sig->ret.kind = func->data.function_def.return_type;
    sig->ret.fun = fun_type_copy (func->data.function_def.return_fun);
    return sig;
}

/**
 * @brief Makes sure the return type of a function is known.
 *
 * - not checked yet: it is checked now,
 * - being checked now: it is recursion, all functions in the circle
 *   must have a written return type (6),
 * - already checked: nothing to do.
 *
 * @param ctx Semantic state
 * @param def Function definition (NODE_FUNCTION_DEF)
 */
static void ensure_checked(SemContext *ctx, ASTNode *def){
    if (def->data.function_def.check_state == 0){
        check_function(ctx, def);
    }
    else if(def->data.function_def.check_state == 1){
        ASTNode *a = ctx->current_function;
        while(a != NULL){
            if (!a->data.function_def.has_explicit_return_type){
                throw_fatal_error(6, "Recursive function must have explicit return type");
            }
            if (a == def){
                break;
            }
            a = a->data.function_def.caller;
        }
    }
}