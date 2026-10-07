//
// Created by Filip on 7. 10. 2026.
//

#include "our_string.h"

#include <stdio.h>
#include <stdlib.h>




// Initializes dynamic string.
String* str_init() {
    String* string_obj = calloc(1, sizeof(String));
    if (string_obj == NULL) {
        fprintf(stderr, "Could not allocate memory. (str_init)\n");
        exit(1);
    }

    char* string_inner = calloc(1, sizeof(char));
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
void str_add(String* string, char c) {
    char* string_new = realloc(string->string, string->len+2);  // +2 bcs new char and terminator

    if (string_new == NULL) {
        fprintf(stderr, "Could not allocate memory. (str_add)\n");
        exit(1);
    }

    string->string = string_new;
    string->string[string->len] = c;
    string->string[string->len+1] = '\0';
    string->len++;
}

// free dynamic string
void str_free(String* string) {
    if (string == NULL) {
        fprintf(stderr, "Input is NULL\n");
        exit(1);
    }

    free(string->string);
    free(string);
}