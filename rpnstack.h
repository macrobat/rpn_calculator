#ifndef RPNSTACK_H
#define RPNSTACK_H
#include <stdlib.h> // for size_t

/*
rpnstack.h    a LIFO (Last In First Out) data structure
example build:
clang -lm  rpnstack.c rpnfunctions.c rpn.c -o rpn

*/

// index is where the next push will be to, also how many elems are used
// allocated size of stk->data is stk->elemsz * stk->nelems
// update shrinkwhen member when resizing. check it in the halffull function
typedef struct {
    void *data;
    size_t elemsz;
    size_t nelems;
    size_t index;
    size_t shrinkwhen;
} stack_t;

static const double stack_shrinklimit     = 0.45; // shrink when 45% full
// static const double stack_shrinkfactor = 0.5 ;
// static const double stack_growfactor   = 2.0 ;

// stack_create(sizeof(<your element type here>));
extern stack_t *stack_create(size_t sz);
extern void stack_destroy(stack_t *stk);

extern size_t stack_elemsize(stack_t *stk); // probably no use
extern size_t stack_size(stack_t *stk);
extern int stack_empty(stack_t *stk);

extern void stack_push(void *itemp, stack_t *stk);
extern void  stack_pop(void *itemp, stack_t *stk);
extern void  stack_top(void *itemp, stack_t *stk);
extern void stack_peek(void *itemp, size_t dataindex, stack_t *stk);

#endif // RPNSTACK_H

