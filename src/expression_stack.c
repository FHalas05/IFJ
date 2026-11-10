//
// Created by Filip Halaš on 27/10/2026.
//

#include "expression_stack.h"
#include "library.h"
#include <stdbool.h>
#include <stdlib.h>

void init_stack(TStack *s) { s->top = NULL; }

bool is_empty(TStack *s) { return s->top == NULL; }

TData top(TStack *s) {
    if (s->top == NULL) {
        throw_fatal_error(99, "Stack is empty!\n");
    }
    return s->top->data;
}

void push(TStack *s, TData d) {
    TSElem *new = (TSElem *)malloc(sizeof(TSElem));
    if (new == NULL) {
        throw_fatal_error(99, "Could not allocate memory!\n");
    }
    new->next = s->top;
    new->data = d;
    s->top = new;
}

void pop(TStack *s) {
    if (s->top != NULL) {
        TSElem *elem = s->top;
        s->top = elem->next;
        free(elem);
    }
}

void clear_stack(TStack *s) {
    while (!is_empty(s)) {
        pop(s);
    }
}
