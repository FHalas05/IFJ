//
// Created by Filip on 7. 10. 2026.
//
// named our_string to avoid confusion with the C string.h library

#ifndef IFJ_STRING_H
#define IFJ_STRING_H
#include <stddef.h>

typedef struct {
    char *string;
    size_t len;
    size_t capacity;
} DynamicString;

DynamicString *dyn_str_init();

void dyn_str_add(DynamicString *string, int c);

// WARNING: the returned string NEEDS to be freed eventually (use token_free_data_if_applicable())
void dyn_str_add_string(DynamicString *dyn_str, const char *str);

// WARNING: the returned string NEEDS to be freed eventually (use token_free_data_if_applicable())
char *dyn_str_get_string_copy(DynamicString *string);

void dyn_str_free(DynamicString *string);

#endif // IFJ_STRING_H
