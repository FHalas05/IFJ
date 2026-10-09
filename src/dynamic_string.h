//
// Created by Filip on 7. 10. 2026.
//
// named our_string to avoid confusion with the C string.h library


#ifndef IFJ_STRING_H
#define IFJ_STRING_H
#include <stddef.h>

typedef struct {
    char* string;
    size_t len;
} DynamicString;

DynamicString* dyn_str_init();

void dyn_str_add(DynamicString* string, int c);

void dyn_str_free(DynamicString* string);

#endif //IFJ_STRING_H
