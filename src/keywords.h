//
// Created by Šimon on 9/10/2026
//

#ifndef IFJ_KEYWORD_H
#define IFJ_KEYWORD_H

#include <stddef.h>
#include <token.h>
#include "our_string.h"

/**
 * @brief checks if string is keyword
 *  
 * @param string checked string
 * 
 * @return token token of identifier or keyword
 */
Token is_keyword(String* string);

#endif // IFJ_KEYWORD_H