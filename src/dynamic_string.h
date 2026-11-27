/**
 * @file dynamic_string.h
 * @brief Dynamic string interface
 *
 * All needed functions for a useful dynamic string
 *
 * @author Filip Holan (xholanf00)
 * @date 7.10.2026
 */

#ifndef IFJ_STRING_H
#define IFJ_STRING_H
#include <stddef.h>

typedef struct {
    char *string;
    size_t len;
    size_t capacity;
} DynamicString;

/**
 * @brief initializes DynamicString
 *
 * @return DynamicString
 */
DynamicString *dyn_str_init();

/**
 * @brief adds char to DynamicString
 *
 * @param string DynamicString
 * @param c char to be added
 *
 * @return void
 */
void dyn_str_add(DynamicString *string, int c);

/**
 * @brief adds char* to DynamicString
 *
 * @param dyn_str DynamicString
 * @param str char* to be added
 *
 * @return void
 */
void dyn_str_add_string(DynamicString *dyn_str, const char *str);

/**
 * @brief creates a copy of a string, so that the original DynamicString can be freed
 *
 * @param string DynamicString
 *
 * @return char* copied string
 */
char *dyn_str_get_string_copy(DynamicString *string);

/**
 * @brief frees DynamicString
 *
 * @param string DynamicString
 *
 * @return void
 */
void dyn_str_free(DynamicString *string);

#endif // IFJ_STRING_H
