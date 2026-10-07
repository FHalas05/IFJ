//
// Created by Filip on 7. 10. 2026.
//
// Tstring to avoid collision with the C string.h library


#ifndef IFJ_STRING_H
#define IFJ_STRING_H
#include <stddef.h>

typedef struct {
    char* string;
    size_t len;
} String;

String* str_init();

void str_add(String* string, char c);

void str_free(String* string);

#endif //IFJ_STRING_H
