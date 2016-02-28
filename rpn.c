#include <stdio.h>
#include "rpnstack.h"
#include "rpnfunctions.h"

/*
rpn.c
a reverse polish notation calculator
*/

const size_t items_display_limit = 60u;

int main(void) {
    stack_t *rpn_stacks[3];
    rpn_stacks[I_STK ] = stack_create(sizeof(RPN_T));   // 0 interactive stack
    rpn_stacks[H_NUMS] = stack_create(sizeof(RPN_T));   // 1 history nums
    rpn_stacks[H_CMDS] = stack_create(sizeof(token_t)); // 2 history cmds

    char *inputbuf = malloc(BUFSIZ); // [8192] here
    int quit = 0;

    printmsg(HELP); // not printmsg_fresh(). let user repeat first help cmd
    while (!quit) {
        // display the separator and the stack
        display(items_display_limit, rpn_stacks);
        // prompt, read input line, operations, print messages
        quit = handle_input(inputbuf, rpn_stacks);
    }

    free(inputbuf);
    stack_destroy(rpn_stacks[I_STK ]);
    stack_destroy(rpn_stacks[H_NUMS]);
    stack_destroy(rpn_stacks[H_CMDS]);

    return 0;
}

