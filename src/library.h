/**
 * @file library.h
 * @brief Interface for a set of useful functions for other modules
 *
 * @author Filip Holan (xholanf00)
 * @date 9.10.2026
 */

#ifndef IFJ_LIBRARY_H
#define IFJ_LIBRARY_H
#include <stdbool.h>

/**
 * @brief checks if a string contains a specific char
 *
 * @param str string to be checked
 * @param c char to be found
 *
 * @return bool is char in string?
 */
bool string_contains(char *str, int c);

/**
 * @brief exits with a specific error value and an error message
 *
 * @param error_number
 * @param error_type char*
 *
 * @return void
 */
void throw_fatal_error(int error_number, char *error_type);

#endif // IFJ_LIBRARY_H
