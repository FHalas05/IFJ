//
// Created by Filip on 9. 10. 2026.
//

#ifndef IFJ_LIBRARY_H
#define IFJ_LIBRARY_H
#include <stdbool.h>

bool string_contains(char *str, int c);
void throw_fatal_error(int error_number, char *error_type);

#endif // IFJ_LIBRARY_H
