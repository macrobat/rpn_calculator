#include <stdlib.h>    // size_t, malloc(), exit()
#include <string.h>    // memcpy()
#include <errno.h>
#include <stdio.h>     // print errors
#include "rpnstack.h"

//  __ helper functions ________________________________________

// from Zed Shaw learn c thehardway
void stack_die(const char *message) {
    if (errno) {
        perror(message);
    } else {
        printf("rpnstack error: %s\n", message);
    }
    exit(EXIT_FAILURE);
}


// before pushing. full if stk->index == stk->nelems
void stack_grow_full(stack_t *stk) {
    if (stk->index < stk->nelems) { return; } // ok, we're done
    size_t new_nelems = 2u * (stk->nelems + 1u);
    stk->data =
        realloc(stk->data, stk->elemsz * new_nelems);
    if (stk->data == NULL) {
        stack_die("Failed to grow stack");
    } else {
        stk->nelems = new_nelems;
    stk->shrinkwhen = (size_t)(stk->nelems * stack_shrinklimit);
    }
}

// after popping
void stack_shrink_halfful(stack_t *stk) {
    if (stk->index > stk->shrinkwhen) {return; } // ok, do nothing
    size_t new_nelems = (stk->nelems + 1u) / 2u;
    stk->data =
        realloc(stk->data, stk->elemsz * new_nelems);
    if (stk->data == NULL) {
        stack_die("Failed to shrink stack");
    }
    stk->nelems = new_nelems;
    stk->shrinkwhen = (size_t)(stk->nelems * stack_shrinklimit);
}

//  __ public functions ________________________________________

stack_t *stack_create(size_t sz) {
    stack_t *tmp = malloc(sizeof(*tmp));
    if (tmp == NULL) {
        stack_die("Failed to create a stack");
    }
    tmp->data = NULL;
    tmp->elemsz = sz;
    tmp->nelems = 0u;
    tmp->index = 0u;
    tmp->shrinkwhen = 0u;
    return tmp;
}

void stack_destroy(stack_t *stk) {
    free(stk->data);
    free(stk);
}

size_t stack_elemsize(stack_t *stk) { // for completeness
    return stk->elemsz;
}

// how many are there == which index will the next one be pushed to?
size_t stack_size(stack_t *stk) {
    return stk->index;
}

// independent of stack_size()
int stack_empty(stack_t *stk) {
    return stk->index == 0u;
}

// index is where the next item will be pushed to. increment index after use
void stack_push(void *itemp, stack_t *stk) {
    stack_grow_full(stk);
    memcpy(stk->data + stk->index++ * stk->elemsz, itemp, stk->elemsz);
}

void stack_pop(void *itemp, stack_t *stk) {
    if (stack_empty (stk)) {
        stack_die("Tried to pop an empty stack");
    }                     // decrement index before use
    memcpy(itemp, stk->data + --stk->index * stk->elemsz, stk->elemsz);
    stack_shrink_halfful(stk);
}

void stack_top(void *itemp, stack_t *stk) {
    if (stack_empty (stk)) {
        stack_die("Tried to top an empty stack");
    }
    memcpy(itemp, stk->data + (stk->index - 1u) * stk->elemsz, stk->elemsz);
}

// stackwise backwards. here, 0u is bottom, index - 1u is top
void stack_peek(void *itemp, size_t dataindex, stack_t *stk) {
    if (stack_empty (stk)) {
        stack_die("Tried to peek an empty stack");
    }
    if (dataindex >= stk->index) {
        stack_die("Tried to peek over top of stack");
    }
    memcpy(itemp, stk->data + dataindex * stk->elemsz, stk->elemsz);
}

