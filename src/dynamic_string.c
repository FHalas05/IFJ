/**
 * @file dynamic_string.c
 * @brief Implementation of a dynamic string
 *
 * All needed functions for a useful dynamic string
 *
 * @author Filip Holan (xholanf00)
 * @date 7.10.2026
 */

#include "dynamic_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 4

// Initializes dynamic string.
DynamicString *dyn_str_init() {
    DynamicString *string_obj = calloc(1, sizeof(DynamicString));
    if (string_obj == NULL) {
        fprintf(stderr, "Could not allocate memory. (str_init)\n");
        exit(1);
    }

    char *string_inner = calloc(1, sizeof(char) * INITIAL_CAPACITY);
    if (string_inner == NULL) {
        free(string_obj);
        fprintf(stderr, "Could not allocate memory. (str_init)\n");
        exit(1);
    }

    string_inner[0] = '\0';
    string_obj->string = string_inner;
    string_obj->len = 0;
    string_obj->capacity = INITIAL_CAPACITY;

    return string_obj;
}

// adds char to dynamic string.
// inputs are expected to not be NULL
void dyn_str_add(DynamicString *string, int c) {
    if (string->len + 2 > string->capacity) {
        string->capacity *= 2;
        char *string_new = realloc(string->string, string->capacity);
        if (string_new == NULL) {
            fprintf(stderr, "Could not allocate memory. (str_add)\n");
            exit(1);
        }
        string->string = string_new;
    }

    string->string[string->len] = c;
    string->string[string->len + 1] = '\0';
    string->len++;
}

// adds char* to dynamic_string
void dyn_str_add_string(DynamicString *dyn_str, const char *str) {
    if (str == NULL || dyn_str == NULL) {
        fprintf(stderr, "Input is NULL\n");
        exit(1);
    }

    for (size_t i = 0; i < strlen(str); i++) {
        dyn_str_add(dyn_str, str[i]);
    }
}

// WARNING: the returned string NEEDS to be freed eventually (use token_free_data_if_applicable())
// creates a copy of a string, so that the original DynamicString can be freed
char *dyn_str_get_string_copy(DynamicString *string) {
    if (string == NULL) {
        fprintf(stderr, "Input is NULL\n");
        exit(1);
    }

    return strdup(string->string);
}

// free dynamic string
void dyn_str_free(DynamicString *string) {
    if (string == NULL) {
        fprintf(stderr, "Input is NULL\n");
        exit(1);
    }

    free(string->string);
    free(string);
}
