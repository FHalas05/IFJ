#include "ast.h"
#include <stdio.h>
#include <stdlib.h>


void ast_array_append(ASTNode ***array, size_t *count, size_t *capacity, ASTNode *new_item){
    if (*count == *capacity){
        size_t new_capacity;
        if (*capacity == 0){
            new_capacity = 4;
        }
        else{
            new_capacity = *capacity * 2;
        }    
        ASTNode** new_array = realloc(*array, new_capacity * sizeof(ASTNode*));
        if(new_array == NULL){
            fprintf(stderr, "reallocation error\n");
            exit(99);
        }
        *array = new_array;
        *capacity = new_capacity;
    }

    (*array)[*count] = new_item;
    (*count)++;
}