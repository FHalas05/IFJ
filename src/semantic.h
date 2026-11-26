/**
 * @file semantic.h
 * @brief Semantic analysis for IFJ26
 *
 * Checks the meaning of the program after the parser built the AST:
 * - all functions and variables are defined before use
 * - nothing is defined twice
 * - types fit together (only Int -> Double is converted automatically)
 * - function calls have the right number and types of arguments
 * - return types are correct (when not written - found)
 *
 * Error codes:
 * - 3 undefined
 * - 4 redefinition
 * - 5 wrong arguments
 * - 6 wrong types
 * - 7 assignment to val or parameter
 *
 * @author Jakub Juřík (xjurikj00)
 * @date 8.11.2026
 */

#ifndef IFJ_SEMANTIC_H
#define IFJ_SEMANTIC_H

#include <stddef.h>

#include "ast.h"
#include "symtable.h"

/**
 * @brief State of the semantic analysis.
 */
typedef struct {
    SymTable *functions;       // all functions
    Scope *current;            // variables visible now
    size_t next_var_id;        // next unique number for a variable
    ASTNode *current_function; // function that is checked now
} SemContext;

/**
 * @brief Runs the whole semantic analysis on the AST.
 *
 * First pass: saves all functions into a table.
 * Second pass: checks the body of every function.
 * On the first error the program ends with the error code.
 *
 * @param program Root of the AST (NODE_PROGRAM)
 */
void semantic_analyze(ASTNode *program);

/**
 * @brief Checks if a value of type actual can be used where expected is needed.
 *
 * True if the types are the same, if expected is Any (print, toStr) or if an Int is used where a
 * Double is needed.
 *
 * @param expected Type what is needed
 * @param actual Type of the actual value
 *
 * @return true if the value can be used
 */
bool types_compatible(DataType expected, DataType actual);

#endif
