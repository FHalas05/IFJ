//
// Created by Filip on 7. 10. 2026.
//

#include "dynamic_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initializes dynamic string.
DynamicString *dyn_str_init() {
    DynamicString *string_obj = calloc(1, sizeof(DynamicString));
    if (string_obj == NULL) {
        fprintf(stderr, "Could not allocate memory. (str_init)\n");
        exit(1);
    }

    char *string_inner = calloc(1, sizeof(char));
    if (string_inner == NULL) {
        free(string_obj);
        fprintf(stderr, "Could not allocate memory. (str_init)\n");
        exit(1);
    }

    string_inner[0] = '\0';
    string_obj->string = string_inner;
    string_obj->len = 0;

    return string_obj;
}

// adds char to dynamic string.
// inputs are expected to not be NULL
void dyn_str_add(DynamicString *string, int c) {
    char *string_new = realloc(string->string, string->len + 2); // +2 bcs new char and terminator

    if (string_new == NULL) {
        fprintf(stderr, "Could not allocate memory. (str_add)\n");
        exit(1);
    }

    string->string = string_new;
    string->string[string->len] = c;
    string->string[string->len + 1] = '\0';
    string->len++;
}


char* dyn_str_get_string_copy(DynamicString *string) {
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
