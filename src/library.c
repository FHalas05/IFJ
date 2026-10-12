//
// Created by Filip on 9. 10. 2026.
//

#include "library.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool string_contains(char *str, int c) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (str[i] == c) {
            return true;
        }
    }
    return false;
}

void throw_fatal_error(int error_number, char *error_type) {
    fprintf(stderr, "FATAL ERROR: %d - %s\n", error_number, error_type);
    exit(1);
}

void throw_nonfatal_error(int error_number, char *error_type) {
    fprintf(stderr, "NONFATAL ERROR: %d - %s\n", error_number, error_type);
}
