//
// Created by Šimon on 9/10/2026
//

#ifndef IFJ_KEYWORD_H
#define IFJ_KEYWORD_H

#include "dynamic_string.h"
#include "token.h"
#include <stddef.h>

/**
 * @brief checks if string is keyword
 *
 * @param string checked string
 *
 * @return token token of identifier or keyword
 */
Token is_keyword(DynamicString *string);

#endif // IFJ_KEYWORD_H
