/**
 * @file generator.h
 * @brief Interface for IFJcode26 generator
 *
 * Declares functions for generating IFJcode26 instructions from the program's AST representation.
 *
 * @author Šimon Hejl (xhejlsi00)
 * @date 4.11.2026
 */

#ifndef IFJ_GENERATOR_H
#define IFJ_GENERATOR_H

#include "ast.h"

/**
 * @brief Generates code in IFJcode26.
 *
 * @param ast AST representing program in IFJ26
 *
 * @return int Exit/status code (0 on success)
 */
int generator(ASTNode *ast);

#endif // IFJ_GENERATOR_H
