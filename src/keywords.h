/**
 * @file keywords.h
 * @brief Interface for determining if a token is T_KEYWORD or T_IDENT
 *
 * @author Šimon Hejl (xhejlsi00)
 * @date 9.10.2026
 */

#ifndef IFJ_KEYWORD_H
#define IFJ_KEYWORD_H

#include "dynamic_string.h"
#include "token.h"
#include <stddef.h>

/**
 * @brief checks if string is keyword and returns the corresponding Token
 *
 * @param string checked string
 *
 * @return token token of identifier or keyword
 */
Token get_token_for_KEYWORD_or_IDENT(DynamicString *string);

#endif // IFJ_KEYWORD_H
