//
// Created by Filip Halaš on 27/10/2026.
//

#ifndef IFJ_EXPRESSION_STACK_H
#define IFJ_EXPRESSION_STACK_H

#include <stdbool.h>

typedef struct {
    char *data; //todo
} TData;

typedef struct tselem {
    TData data;
    struct tselem *next;
} TSElem;

typedef struct {
    TSElem *top;
} TStack;

void init_stack(TStack *s);
bool is_empty(TStack *s);
TData top(TStack *s);
void push(TStack *s, TData d);
void pop(TStack *s);
void clear_stack(TStack *s);


#endif // IFJ_EXPRESSION_STACK_H
