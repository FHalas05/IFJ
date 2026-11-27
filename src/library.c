/**
 * @file library.c
 * @brief Implementation of a set of useful functions for other modules
 *
 * @author Filip Holan (xholanf00)
 * @date 9.10.2026
 */

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
    exit(error_number);
}

void throw_nonfatal_error(int error_number, char *error_type) {
    fprintf(stderr, "NONFATAL ERROR: %d - %s\n", error_number, error_type);
}
