#ifndef RPNSTACK_H
#define RPNSTACK_H
#include <stdlib.h> // for size_t

// rpnstack.h
// a LIFO (Last In First Out) data structure

// index is where the next push will be to, also how many elems are used
// allocated size of stk->data is stk->elemsz * stk->nelems
// update shrinkwhen member when resizing. check it in the halffull function
typedef struct {
    size_t elemsz;
    size_t nelems;
    size_t index;
    size_t shrinkwhen;
    void *data;
} stack_t;

// for hysteresis sake
static const double stack_shrinklimit = 0.41; // shrink when 41% full
// static const double stack_shrinkfactor = 0.5 ;
// static const double stack_growfactor   = 2.0 ;

// stack_create(sizeof(<element type>));
stack_t *stack_create(size_t sz);
void stack_destroy(stack_t *stk);

size_t stack_elemsize(stack_t *stk); // probably no use
size_t stack_size(stack_t *stk);
int stack_empty(stack_t *stk);

void stack_push(void *itemp, stack_t *stk);
void  stack_pop(void *itemp, stack_t *stk);
void  stack_top(void *itemp, stack_t *stk);
void stack_peek(void *itemp, size_t dataindex, stack_t *stk);
void stack_roll(int direction, stack_t *stk);

#endif // RPNSTACK_H

